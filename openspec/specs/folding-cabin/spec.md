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
Every room SHALL have doors on at least three walls. The front of the Hearth Room SHALL lead outdoors. Rear and right doors SHALL start closed. Opening an unexplored door SHALL generate and reveal one room. Both sides SHALL share its door state. Layouts, connections, windows and interior doors SHALL survive dismissal, destination changes and a restart. The renderer SHALL hold geometry for the active room and its open doorway views.

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
Local phenomena and both local sightings SHALL be optional and default off. The hiker SHALL be able to switch the phenomenon and each sighting independently. They SHALL be observable only from the shelter, including its balconies, and SHALL never detect, pursue, reach or injure the hiker.

#### Scenario: Watch from a balcony
- **WHEN** the hiker enables an encounter, opens a balcony door and goes outside onto the deck
- **THEN** the spectacle remains visible at a safe distance and switching it off removes it

### Requirement: Destination shelters
Each destination SHALL select an externally authored shelter with a recognizable silhouette, materials and supports appropriate to the terrain. The Sahara SHALL use a pitched canvas tent. The Moon and the four alien worlds SHALL use pressure capsules with distinct equipment. Changing shelters SHALL preserve the shared floor plan and its saved doors, windows and blinds.

#### Scenario: Take the house from a dune to a crater
- **WHEN** the hiker summons the house at Erg Chebbi, opens a room and visits Mare Imbrium
- **THEN** the canvas tent gives way to a lunar capsule with the same discovered room
- **AND** opening the external hatch reveals the furnished Hearth Room

### Requirement: Destination lookout encounters
Each destination SHALL provide three selectable phenomena and two independently selectable sightings that fit the place. The interface, rendered effects and sound SHALL agree with the selected destination. Marine visitors SHALL occupy open water. The Moon SHALL support airless phenomena and geological or mechanical sightings. Encounters SHALL appear only while the hiker is inside the shelter or on its balcony.

#### Scenario: Watch a lunar impact shower
- **WHEN** the hiker chooses micrometeor impacts in the lunar capsule
- **THEN** impact debris follows ballistic arcs above the ground under an airless sky
- **AND** the hiker hears no atmospheric thunder

#### Scenario: Change destination with encounters enabled
- **WHEN** the hiker travels from one destination to another with sightings enabled
- **THEN** the controls and sightings use the new destination's catalogue
- **AND** the hiker may turn each sighting off independently
