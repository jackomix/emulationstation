# Handoff Report: R1 & R2 Investigation

## 1. Observation
I directly observed the following in the codebase:
- **Missing Renderer Matrix Bind in `HomeView::render`**:
  In `es-app/src/views/HomeView.cpp`, line 373:
  ```cpp
  void HomeView::render(const Transform4x4f& parentTrans)
  {
      // Draw screen background gradient
      Renderer::drawRect(0.0f, 0.0f, mSize.x(), mSize.y(), 0x1A1A2EFF, 0x16213EFF, false);
  ```
  No call to `Renderer::setMatrix(parentTrans)` is performed before `Renderer::drawRect` is executed.
- **Unregistered child `mHomeView` in `ViewController` constructor**:
  In `es-app/src/views/ViewController.cpp`, line 90-95:
  ```cpp
  #ifdef ES_HOME_VIEW
      mHomeView = std::make_shared<HomeView>(window);
      mYTransitioning = false;
      mCameraTargetY = 0.0f;
  #endif
  ```
  `mHomeView` is constructed but `addChild(mHomeView.get())` is never called.
- **Culling Bypass Check**:
  In `es-app/src/views/ViewController.cpp`, line 1118-1126:
  ```cpp
  #ifdef ES_HOME_VIEW
  		// Draw homeview with culling
  		if (mHomeView)
  		{
  			Vector3f guiStart = mHomeView->getPosition();
  			Vector3f guiEnd = mHomeView->getPosition() + Vector3f(mHomeView->getSize().x(), mHomeView->getSize().y(), 0);
  			if (guiEnd.x() > viewStart.x() && guiEnd.y() > viewStart.y() && guiStart.x() < viewEnd.x() && guiStart.y() < viewEnd.y())
  				mHomeView->render(trans);
  		}
  ```
  If `mHomeView->getSize()` is `(0, 0)`, then `guiEnd` is `(0, 0, 0)`. The condition `guiEnd.x() > viewStart.x()` (which checks `0.0f > 0.0f` at boot) evaluates to `FALSE`, culling the view.
- **Tab Header Overlay Render Condition**:
  In `es-app/src/views/ViewController.cpp`, line 1155-1157:
  ```cpp
  #ifdef ES_HOME_VIEW
  	if (mState.viewing == HOME_VIEW || mState.viewing == SYSTEM_SELECT)
  	{
  ```
  The tab header overlay is drawn unconditionally when `viewing` is either `HOME_VIEW` or `SYSTEM_SELECT`.

---

## 2. Logic Chain
1. **R1: Why the background gradient renders blank/black**:
   - In EmulationStation, the OpenGL renderer state persists between draws and frames. In `Window::render()`, no initial identity matrix is bound to the Renderer before executing child rendering.
   - Consequently, the modelview matrix remains set to whatever translation/scale was last used (e.g. from the help prompts or system logos rendered at the end of the previous frame).
   - In `HomeView::render` (Observation 1), `Renderer::drawRect` is called to draw the screen-sized dark blue background gradient, but without calling `Renderer::setMatrix(parentTrans)` beforehand, the rectangle is drawn under the previous dirty matrix state. This projects the background rectangle completely off-screen, resulting in a black background.
2. **R1: Why the widgets and tiles are also blank**:
   - At startup, `ViewController::ViewController` is constructed before `Window::init`. Thus, `Renderer::getScreenWidth()` and `Renderer::getScreenHeight()` return `0`.
   - `HomeView` size is initially initialized to `(0, 0)` in its constructor.
   - When transitioning to `HomeView` on boot (`goToStart(true)` -> `goToHomeView(true)`), a 1ms LambdaAnimation is started, making `isAnimationPlaying(0)` evaluate to `TRUE`.
   - While `isAnimationPlaying(0)` is true, `ViewController::render` falls into the `else` block containing the culling check (Observation 3).
   - Because `mHomeView->getSize()` is initially `(0, 0)`, the culling check `guiEnd.x() > viewStart.x()` evaluates to `0.0f > 0.0f`, which is `false`. Thus, `mHomeView->render` is bypassed during animation frames.
   - Adding `addChild(mHomeView.get())` in `ViewController`'s constructor ensures `mHomeView` is registered in the layout and update tree. Returning early in `onSizeChanged` when size is `0` avoids retrieving font sizes or sizing cards to `0`.
3. **R2: Why the tab header overlay draws on System View**:
   - The condition `mState.viewing == HOME_VIEW || mState.viewing == SYSTEM_SELECT` (Observation 4) triggers rendering of the tab header overlay whenever the current view mode is either of these two.
   - When the camera finishes panning down to System View, `mState.viewing` remains `SYSTEM_SELECT`. Consequently, the overlay continues to render on top of System View.
   - During the transition from `HOME_VIEW` to `SYSTEM_SELECT`, the camera's translation Y ranges from `0.0f` down to `-(float)Renderer::getScreenHeight()`.
   - By modifying the check to `mState.viewing == HOME_VIEW || (mState.viewing == SYSTEM_SELECT && mCamera.translation().y() > -(float)Renderer::getScreenHeight() + 1.0f)`, the overlay is drawn when viewing `HOME_VIEW`, or while the camera is panning (Y translation is greater than the System View offset), but is hidden immediately when the camera finishes panning to `SYSTEM_SELECT` (Y translation becomes exactly `-ScreenHeight`).

---

## 3. Caveats
- No caveats. The behavior was traced exactly from the source files using filesystem tools.

---

## 4. Conclusion
The blank `HomeView` screen is caused by a missing `Renderer::setMatrix(parentTrans)` in `HomeView::render` which projects the background offscreen, combined with a culling bypass due to `mHomeView` having a size of `(0, 0)` initially.
The tab header overlay drawing on `SystemView` is caused by an over-broad rendering condition in `ViewController::render`.

Recommended Fix Strategy:
Apply the modifications in the attached `.patch` file:
1. Call `Renderer::setMatrix(parentTrans)` at the beginning of `HomeView::render`.
2. Add an early return to `HomeView::onSizeChanged` when size is `(0, 0)`.
3. Call `addChild(mHomeView.get())` in `ViewController`'s constructor.
4. Refine the rendering condition of the tab header overlay in `ViewController::render` using the camera's Y translation value.

---

## 5. Verification Method
1. Inspect the target files:
   - `/Users/jacko/Documents/myEmulationStation/es-app/src/views/HomeView.cpp`
   - `/Users/jacko/Documents/myEmulationStation/es-app/src/views/ViewController.cpp`
2. Apply the patch from `/Users/jacko/Documents/myEmulationStation/.agents/explorer_r1_r2/fixes.patch` using `git apply`.
3. Compile the project.
4. Run the EmulationStation executable and verify:
   - On boot, `HomeView` displays correctly with the dark blue gradient background, avatar, profile, clock, battery widgets, and the 5 dashboard tiles (Browse, Achievements, Profiles, Settings, Continue Playing).
   - Pressing `R1` or `pagedown` transitions to `SystemView`. During transition, the tab header overlay stays visible. Once transition finishes, the tab header overlay disappears.
   - Pressing `L1` or `pageup` on `SystemView` transitions back to `HomeView`, during which the tab header overlay immediately reappears and stays visible.
