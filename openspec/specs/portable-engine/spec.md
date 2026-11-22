# Portable engine

## Purpose

Allow the same game to run on desktop, the mobile web and immersive headsets while keeping the world editable.

## Requirements

### Requirement: Shared engine
Windows, Linux and Web builds SHALL use the same C11 simulation, Lua scripts, SDL3 and OpenGL-family renderer. Art, shaders, sound and scripts SHALL load from loose files beside the application.

#### Scenario: Edit the world
- **WHEN** an author edits the distributed Lua world definition and reloads
- **THEN** its settings take effect without rebuilding the binary

### Requirement: Device controls
The web build SHALL support mouse capture on desktop browsers with Pointer Lock, touch controls on touch devices and stereo views from the headset pose in an available immersive WebXR session.

#### Scenario: Look with a captured mouse
- **WHEN** a desktop player clicks the landscape with the primary mouse button
- **THEN** mouse movement turns the view without dragging, and pressing Escape or opening a game panel releases the cursor

#### Scenario: Look with the cursor free
- **WHEN** the cursor is free or the browser declines capture
- **THEN** dragging the landscape and using the left and right arrow keys still turn the view

#### Scenario: Walk on a phone
- **WHEN** a touch player uses the movement pad and drags the scenery
- **THEN** the player walks and turns without a keyboard

#### Scenario: View the ridge in a headset
- **WHEN** a supported headset starts an immersive session
- **THEN** each eye uses its own view, viewport and projection and ending the session restores the flat view

### Requirement: Tested playtest distribution
The project SHALL run unit tests with a 95 percent line coverage gate, exercise the compiled game automatically and put only the web distribution on gh-page.

#### Scenario: Build a playtest
- **WHEN** a verified web build is published
- **THEN** its page, WebAssembly engine and loose assets can load from a repository subpath
