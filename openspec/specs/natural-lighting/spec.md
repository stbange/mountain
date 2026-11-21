# Natural lighting

## Purpose

Give hikers changing light to photograph on the trail and a sheltered place to watch it from the cabin.

## Requirements

### Requirement: Moving sunlight and cast shadows

The visible sun SHALL determine direct lighting and cast shadows for terrain, trees, wildlife, trail landmarks and the cabin. Skylight SHALL keep shaded alpine trails legible. Foliage silhouettes SHALL preserve gaps in the visible canopy.

#### Scenario: Wait beside the trail

- **WHEN** the hiker watches a clear ridge from morning to evening
- **THEN** its illuminated slopes change sides and a nearby object's shadow points away from the sun
- **AND** a shaded valley can remain visible beneath a sunlit summit

### Requirement: Climate and world lighting

Destinations SHALL expose sky fill, ground bounce, cloud influence and shadow softness in loose Lua content. Visible clouds SHALL attenuate sunlight on the ground beneath their projected shadows.

#### Scenario: Change worlds

- **WHEN** the hiker travels from a cloudy alpine meadow to Saharan dunes and then the Moon
- **THEN** the meadow receives diffuse daylight, the dunes receive warm ground fill, and the lunar surface has sharply edged shadows and no passing cloud cover

### Requirement: Sheltered cabin lighting

The roof, door and window frames SHALL cast shadows. The player SHALL be able to open and close the front door from either side of the entrance. The stove and bedside lamp SHALL illuminate nearby furnishings after dusk.

#### Scenario: Close the door in afternoon sunlight

- **WHEN** the hiker closes the sun-facing front door from the Hearth Room
- **THEN** the light patch from the door disappears, light through the windows remains, and the closed threshold prevents crossing until reopened

### Requirement: Consistent viewpoints

Shadow placement SHALL be anchored to the world and shared by the ordinary view, lake reflections, photographs and both WebXR eyes.

#### Scenario: Photograph or turn toward the same view

- **WHEN** the hiker photographs a scene, turns their head in stereo, or crosses a world-coordinate boundary
- **THEN** the shadows stay attached to the same objects and terrain for the same time and position
