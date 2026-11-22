# Field kitchen

## Purpose

Let hikers gather local produce, discover recipes and take a drink and a plate of food to a balcony overlooking the current destination.

## Requirements

### Requirement: Regional and travelling recipes

The game SHALL provide 20 distinct forageable ingredients and 24 named local recipes at each of the twelve destinations, plus 24 travelling recipes combining ingredients from different destinations. The 312 named recipes SHALL include 54 drinks. Loose Lua files SHALL define ingredient identities, habitat preferences, ingredients, techniques, servings and visual models.

#### Scenario: Browse across worlds

- **WHEN** the hiker opens the cookbook
- **THEN** the local shelf shows the destination’s recipes and the travelling shelf offers combinations using produce from more than one destination

### Requirement: Repeatable foraging and a shared pantry

A given seed and world location SHALL produce repeatable harvest patches. A nearby hiker SHALL be able to gather a ripe patch once, with a bonus during its peak season. Patches SHALL replenish after another kilometre of hiking or a rest in the shelter. The pantry, discoveries, packed servings, remembered patches, pinned recipe and unlocked table settings SHALL persist across destination changes and game restarts.

#### Scenario: Return to a harvested berry patch

- **WHEN** the hiker gathers berries, visits another destination and returns to the same patch before it has regrown
- **THEN** the berries are still in the pantry and a second harvest is unavailable

#### Scenario: Seek produce in the winter garden

- **WHEN** the hiker pins a recipe and tends the shelter’s winter garden
- **THEN** the garden supplies one of that recipe’s local ingredients, favouring the one with the lowest stock

### Requirement: Cooking and experimentation

The Hearth Room kitchen SHALL make a named recipe when the pantry contains its gathered ingredients. A successful preparation SHALL consume those ingredients once, record the discovery, and pack portions on the appropriate side of the tray. A method and one to three distinct gathered ingredients SHALL produce a matching recipe or a field infusion or bowl.

#### Scenario: Make two refreshments on a first visit

- **WHEN** the hiker makes Hearth coffee and Ranger’s oat cookies with the kitchen cupboard supplies
- **THEN** the tray holds two sips of coffee and three cookies, with both refreshments available together

### Requirement: Dining on a protected balcony

A hiker SHALL be able to carry the tray through the winter garden to the balcony, sit in an outward-facing chair, look around, photograph their drink and food, sip or eat separately and stand up. The shelter’s protection SHALL extend to a seated hiker. A seated photograph featuring the destination’s house dish SHALL unlock its table setting.

#### Scenario: Photograph and finish a balcony meal

- **WHEN** a seated hiker photographs coffee and cookies and eats the last cookie
- **THEN** the photograph retains the mug and cookies, the plate clears and the remaining coffee can still be sipped

### Requirement: Platform controls

Keyboard, web touch and WebXR controller input SHALL support gathering, preparing a recipe, carrying the tray, sitting, drinking, eating and standing.

#### Scenario: Play with touch controls

- **WHEN** a hiker opens the cookbook on a touchscreen, brews coffee, bakes cookies and carries the tray outside
- **THEN** on-screen controls allow them to sit down, sip, eat and stand without a physical keyboard
