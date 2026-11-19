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

### Requirement: Marked scenic hike
For each mountain the engine SHALL generate a continuous, seeded footpath over dry terrain. It SHALL choose an accessible lakeside viewpoint and an elevated lookout, render the tread against the terrain, keep vegetation off the path, and mark the two stops on the web trail map.

#### Scenario: Follow a hike end to end
- **WHEN** a hiker follows the marked tread from the trailhead through both map stops
- **THEN** the route is walkable, remains on land and reaches both cairns in order

#### Scenario: Return in another season
- **WHEN** a hiker revisits a seed or changes its conditions
- **THEN** the terrain, path geometry and signed stops remain in the same locations

### Requirement: Surveyed mountain scenery
The game SHALL offer a named Taggart Lake destination made from USGS 3DEP elevations alongside the procedural seeds. The game SHALL load the survey from a loose asset, indicate the actual elevation, and distinguish lake shoreline from dry valley floor.

#### Scenario: Explore Wyoming
- **WHEN** the hiker chooses Taggart Lake from the web trail map or presses B on native
- **THEN** the hike opens on the eastern shore with the surveyed Teton skyline, a generated walk and a map distance expressed in real-world metres

### Requirement: Selectable and cycling conditions
A hiker SHALL be able to hold a chosen season or weather indefinitely while outdoors or indoors and enable automatic changes independently for each. The web interface SHALL expose those controls by name on desktop and touch. Native and supported XR controllers SHALL provide shortcuts for seasonal and weather selection.

#### Scenario: Hold summer while weather changes
- **WHEN** the hiker chooses Summer and enables changing weather
- **THEN** the weather can advance while summer and the selected hiking route persist

#### Scenario: Winter snowfall
- **WHEN** the hiker selects winter and precipitation
- **THEN** the weather choice reads Snow and snow falls outdoors

### Requirement: Seasonal alpine ecology
The game SHALL place seasonal flowers and seed heads in reproducible meadow patches. Butterflies and bumblebees SHALL visit those patches during suitable daylight, alongside the existing browsing deer and birds.

#### Scenario: A summer meadow
- **WHEN** a hiker visits a flowering patch during a clear summer afternoon
- **THEN** insects can be seen flying, hovering or settling by the flowers and their movement changes over time

#### Scenario: A cold or wet meadow
- **WHEN** winter, rain, mist or night arrives
- **THEN** butterfly and bumblebee flights cease and the underlying path stays visible
