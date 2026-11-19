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
The main room SHALL contain a bed, kitchen, writing desk and bookcase. The cabin SHALL generate additional windowed rooms on demand with bounded memory and support a return to the entrance.

#### Scenario: Wander home
- **WHEN** the player visits successive rooms and returns through the previous doors
- **THEN** the same rooms and their surrounding views are available in reverse order

#### Scenario: Dismiss after use
- **WHEN** the player exits and dismisses the cabin
- **THEN** the landscape reclaims the site
