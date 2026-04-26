#!/usr/bin/env python3
import urllib.request
import os
import subprocess

def test_url(name, url):
    print(f"  Testing {name}...")
    try:
        # LAHEE requires POST for dorequest.php
        # We send an empty body to trigger the route
        req = urllib.request.Request(url, data=b"r=laheeinfo", method="POST")
        response = urllib.request.urlopen(req, timeout=2)
        code = response.getcode()
        if code == 200:
            print(f"    [ OK ] Success (200)")
            return True
        else:
            print(f"    [ !! ] Failed with code: {code}")
    except Exception as e:
        # If we get a 404, it means the server is alive but the PATH is wrong
        if "404" in str(e):
            print(f"    [ 404 ] Server reached, but path is REJECTED.")
        else:
            print(f"    [FAIL] Error: {e}")
    return False

def test_connection():
    print("--- LAHEE PADDING COMPATIBILITY TEST (POST Mode) ---")
    
    # 1. Basic Process Check
    try:
        ps = subprocess.check_output(["ps", "aux"]).decode().lower()
        if "lahee" not in ps:
            print("\n[!!] WARNING: LAHEE Server process not detected in memory.")
    except:
        pass

    # 2. Test Variations
    print("\nChecking which paths the server accepts via POST:")
    
    base = "http://127.0.0.1:8000"
    methods = [
        ("Standard",      f"{base}/dorequest.php"),
        ("Slash Padding", f"{base}////dorequest.php"),
        ("Dot Padding",   f"{base}/./././dorequest.php"),
        ("Zero Port",     "http://127.0.0.1:00000008000/dorequest.php"),
        ("Zero IP",       "http://127.000.000.001:8000/dorequest.php")
    ]

    results = []
    for name, url in methods:
        results.append(test_url(name, url))

    print("\n--- Summary ---")
    for i in range(len(methods)):
        status = "[ PASS ]" if results[i] else "[ FAIL ]"
        print(f"{status} {methods[i][0]}")

    print("\n--- Recommendation ---")
    if results[0]:
        print("The server is ALIVE and accepting standard POST requests.")
        if not results[1] and not results[2]:
            print("CRITICAL: The server is rejecting padded paths!")
            print("We must update the server to accept slashes or find a perfect-length hostname.")
    else:
        print("The server is not responding to standard requests. Check lahee.log.")

    print("\n--- Diagnostic Complete ---")

if __name__ == "__main__":
    test_connection()
