# Folding cabin

## Purpose

Provide a familiar, sheltered place anywhere on the trail, with more rooms to explore beyond the entrance.

## Requirements

### Requirement: A grounded cabin
The player SHALL be able to summon and dismiss a cabin outdoors. Its foundation and vegetation clearance SHALL fit the local terrain. A closed door SHALL prevent entry, and opening it SHALL reveal the furnished interior from the outside.

#### Scenario: Arrive at the cabin
- **WHEN** the player summons a cabin, approaches the entrance and opens its door
- **THEN** the player sees into the main room and can cross the threshold

### Requirement: Unlimited connected rooms
The main room SHALL contain a bed, kitchen, writing desk and bookcase. The cabin SHALL generate additional windowed rooms on demand with bounded rendering memory and support a return to the entrance.

#### Scenario: Wander home
- **WHEN** the player visits successive rooms and returns through the previous doors
- **THEN** the same rooms and their surrounding views are available in reverse order

#### Scenario: Dismiss after use
- **WHEN** the player exits and dismisses the cabin
- **THEN** the landscape reclaims the site

### Requirement: Persistent branching rooms
Every room SHALL have doors on at least three walls. The front of the Hearth Room SHALL lead outdoors. Its rear and right doors, and those of each discovered room, SHALL start closed. Opening an unexplored door SHALL create one connected room before revealing it. Both sides SHALL share the door state. Discovered layouts, connections, windows and interior doors SHALL survive dismissal, destination changes and a restart. The renderer SHALL keep only the current room and open doorway views in memory as geometry.

#### Scenario: Open a branch
- **WHEN** a hiker walks up against an unexplored door
- **THEN** it blocks passage and the room count stays the same
- **WHEN** the hiker opens the door, views the room, crosses and closes the door from inside
- **THEN** returning to the same door reveals the same room and shared door state

#### Scenario: Return to the hearth
- **WHEN** the hiker uses the hearth shortcut from a distant room, a balcony or the trail
- **THEN** the hiker arrives inside the entrance room with discoveries intact

### Requirement: Habitable rooms and windows
Each window SHALL have an independently operable glass casement and blind. The cabin SHALL contain rooms of different dimensions, including a swimming hall, a roofed garden and walkable, railed balconies. Every window SHALL look out on the current destination.

#### Scenario: Remember a window
- **WHEN** a hiker opens a pane, lowers its blind and later revisits that room
- **THEN** that pane stays open, its blind stays down and neighbouring windows keep their own settings

### Requirement: Optional sheltered encounters
Distant thunderstorms, snowstorms, heavy fog, predators and giants SHALL be optional and default off. The hiker SHALL be able to switch weather effects, predators and giants independently. They SHALL be observable only from the shelter, including its balconies, and SHALL never detect, pursue, reach or injure the hiker.

#### Scenario: Watch from a balcony
- **WHEN** the hiker enables an encounter, opens a balcony door and goes outside onto the deck
- **THEN** the spectacle remains visible at a safe distance and switching it off removes it
