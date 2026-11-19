# Alpine hiking

## Purpose

Let hikers explore a reproducible mountain landscape, discover wildlife and keep photographs at their own pace.

## Requirements

### Requirement: Living mountain
The game SHALL generate a traversable alpine landscape from a seed and compose its terrain, water, flora and fauna with seasonal colours, weather, sun, moon and a day/night cycle.

#### Scenario: Repeat a trail
- **WHEN** a hiker starts the same seed twice
- **THEN** the starting location, peaks and planted landscape agree

#### Scenario: Change the light
- **WHEN** the hiker changes the season, weather or time
- **THEN** the same mountain reflects the new conditions

### Requirement: Field photography
A hiker SHALL be able to capture a clean image of the rendered scene and retain or download it after taking the photograph.

#### Scenario: Revisit a photograph
- **WHEN** a web hiker opens the album after a reload
- **THEN** a saved photograph can be viewed and downloaded as a PNG
