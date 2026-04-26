#!/usr/bin/env python3
import sys
import os
import json
import urllib.request
import urllib.parse

# Set up logging
LOG_FILE = "lahee_ui.log"
def log(msg):
    with open(LOG_FILE, "a") as f:
        f.write(f"{msg}\n")
    print(msg)

log(f"Python version: {sys.version}")
log(f"Python executable: {sys.executable}")
log(f"Python path: {sys.path}")
log(f"Current directory: {os.getcwd()}")

try:
    import pygame
    log(f"Pygame version: {pygame.version.ver}")
except Exception as e:
    log(f"CRITICAL: Failed to import pygame: {e}")
    # We'll catch this again in main() for the full traceback

# R36S Screen Resolution
WIDTH = 640
HEIGHT = 480

def fetch_lahee_info():
    try:
        API_URL = "http://127.0.0.1:8000/dorequest.php"
        req = urllib.request.Request(API_URL, data=b"r=laheeinfo", headers={'Content-Type': 'application/x-www-form-urlencoded'})
        with urllib.request.urlopen(req, timeout=3) as response:
            return json.loads(response.read().decode())
    except Exception as e:
        log(f"Error fetching LAHEE info: {e}")
        return None

def main():
    try:
        import pygame
        pygame.init()
        if not pygame.display.get_init():
            log("Pygame display failed to initialize")
            return

        screen = pygame.display.set_mode((WIDTH, HEIGHT))
        pygame.display.set_caption("LAHEE UI")
        
        try:
            font = pygame.font.SysFont("monospace", 20)
            large_font = pygame.font.SysFont("monospace", 30)
        except Exception as e:
            log(f"Error loading fonts: {e}")
            # Fallback to default font
            font = pygame.font.Font(None, 24)
            large_font = pygame.font.Font(None, 36)

        clock = pygame.time.Clock()
        running = True

        log("Fetching info from server...")
        info = fetch_lahee_info()
        
        if not info:
            state = "ERROR"
            log("State: ERROR (Could not connect to server)")
        else:
            state = "LIST"
            log("State: LIST")
            
        games = info.get("games", []) if info else []
        users = info.get("users", []) if info else []
        
        selected_game_idx = 0
        
        while running:
            screen.fill((30, 30, 30))
            
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    running = False
                elif event.type == pygame.KEYDOWN:
                    if event.key == pygame.K_ESCAPE:
                        running = False
                    elif event.key == pygame.K_UP:
                        selected_game_idx = max(0, selected_game_idx - 1)
                    elif event.key == pygame.K_DOWN:
                        selected_game_idx = min(len(games) - 1, selected_game_idx + 1)
                    elif event.key == pygame.K_RETURN:
                        pass

            if state == "ERROR":
                text = font.render("Could not connect to LAHEE Server.", True, (255, 100, 100))
                screen.blit(text, (20, 20))
                text2 = font.render("Make sure LAHEE is running in the background.", True, (200, 200, 200))
                screen.blit(text2, (20, 60))
                text3 = font.render("Press B (ESC) to exit.", True, (200, 200, 200))
                screen.blit(text3, (20, 100))
            elif state == "LIST":
                title = large_font.render("LAHEE Games", True, (255, 255, 255))
                screen.blit(title, (20, 20))
                
                if not games:
                    text = font.render("No games found in LAHEE Data folder.", True, (200, 200, 200))
                    screen.blit(text, (20, 80))
                else:
                    for i, game in enumerate(games):
                        color = (255, 255, 100) if i == selected_game_idx else (200, 200, 200)
                        text = font.render(f"{game.get('Title', 'Unknown')} (ID: {game.get('ID', '0')})", True, color)
                        screen.blit(text, (20, 80 + i * 30))
                        
                    if len(users) > 0 and len(games) > 0:
                        current_user = users[0]
                        game_id = str(games[selected_game_idx].get("ID", ""))
                        ug_data = current_user.get("GameData", {}).get(game_id, {})
                        ach_dict = ug_data.get("Achievements", {})
                        
                        unlocked_count = len([a for a in ach_dict.values() if a.get("Status", 0) > 0])
                        sets = games[selected_game_idx].get("AchievementSets", [])
                        total_count = len(sets[0].get("Achievements", [])) if sets else 0
                        
                        stat_text = font.render(f"Unlocked: {unlocked_count} / {total_count}", True, (100, 255, 100))
                        screen.blit(stat_text, (20, 400))
                
                text_footer = font.render("Press B (ESC) to exit", True, (150, 150, 150))
                screen.blit(text_footer, (20, 440))

            pygame.display.flip()
            clock.tick(30)

        pygame.quit()
    except Exception as e:
        log(f"FATAL ERROR: {e}")
        import traceback
        log(traceback.format_exc())
        pygame.quit()
        sys.exit(1)

if __name__ == "__main__":
    if os.path.exists(LOG_FILE):
        os.remove(LOG_FILE)
    log("LAHEE UI Starting...")
    main()
