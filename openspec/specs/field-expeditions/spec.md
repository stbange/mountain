# Field expeditions

## Purpose

Give hikers a reason to look closely at a landscape, from the snow saddle on Larkspur Ridge to the rim of a crater on the Moon.

## Requirements

### Requirement: A marked alpine summit

Procedural alpine mountains SHALL offer a distinct, traversable route from Ridge Lookout across a high, snowy saddle to a summit. The fork SHALL also connect to the continuing backcountry trail. The two trails SHALL share the terrain, trail markings and collision model used by the hiker.

#### Scenario: Climb and return

- **WHEN** a hiker follows the summit branch in summer
- **THEN** switchbacks climb over 440 game metres from the fork, through a saddle flanked by higher ground, onto a snow-covered crown
- **AND** the entire marked tread is dry and walkable in both directions
- **AND** the web map can show the fork, high pass and summit together

#### Scenario: Traverse further ranges

- **WHEN** the hiker continues kilometres along the endless alpine trail
- **THEN** the marked ground repeatedly climbs over snowy high passes and descends into valleys, with seed-specific elevations and continuous grades across generation boundaries

#### Scenario: Lose footing

- **WHEN** a hiker approaches an exposed drop on foot and continues downhill over the edge
- **THEN** the hiker receives an edge warning, experiences a short fall and returns to their most recent safe trail position
- **AND** their field discoveries and summoned cabin survive the fall

### Requirement: An atlas of distinct terrains

The game SHALL support five geographically inspired Earth destinations in addition to procedural alpine country and the measured Taggart Lake hike. It SHALL also offer the Moon and four fictional destinations. Every destination SHALL generate reproducible terrain and a continuous marked trail beyond the opening route, with locally appropriate scenery and sky.

#### Scenario: Explore the Earth destinations

- **WHEN** the hiker visits Sarakiniko on Milos, Erg Chebbi in the Sahara, Reynisfjara in Iceland, Canyonlands in Utah or the Li River at Guilin
- **THEN** they encounter respectively pale coastal tuff and sea arches, dunes and recurring palm oases, black sand and basalt columns, layered sandstone mesas and arches, or riverbanks lined with bamboo and karst towers

#### Scenario: Explore beyond Earth

- **WHEN** the hiker visits Mare Imbrium, Prism Choir, Lantern Mycelium, The Hanging Sea or Emberglass Caldera
- **THEN** they encounter respectively lunar crater rims and Earth in an airless sky, faceted crystal highlands, luminous mushroom groves, floating reefs beneath a ringed planet, or volcanic fissures and molten pools

#### Scenario: Choose and extend a hike

- **WHEN** a player chooses a destination in the web atlas or cycles to it on desktop
- **THEN** the game opens its canonical seed, with its own landmarks, sky and ambient sound
- **AND** the new regional terrains can generate alternate seeds and continue far beyond the starting route
- **AND** doors, cabin windows, field photography and touch or headset controls work in the destination

### Requirement: Optional field discoveries

Each destination SHALL offer a three-stage environmental discovery. A distinct first landmark SHALL point to a second physical clue, whose solution SHALL reveal a discoverable treasure in the landscape. The hints and markers SHALL allow the hiker to complete a discovery without a quest pin, timer or health penalty.

#### Scenario: Sight, listen or read the light

- **WHEN** the hiker reads a sighting clue while looking along its markers, reads a dusk clue in evening light, or sounds a set of three pipes from shortest to tallest
- **THEN** the final bearing and the local treasure are revealed
- **AND** an unsuccessful attempt provides a clue and can be retried
- **AND** visual feedback accompanies the chimes so the hike is playable with sound muted

#### Scenario: Keep a discovery

- **WHEN** the hiker reads a sign or discovers the treasure, changes destinations and later reopens the game
- **THEN** progress for each destination is restored from the local field journal
- **AND** the field guide can show the current clue or the recorded discovery

### Requirement: Editable expedition content

Destination identities, palettes, discovery text, landmark meshes and ambient sounds SHALL load from external assets alongside the game.

#### Scenario: Playtest an edited destination

- **WHEN** an author edits a destination's Lua description or landmark geometry and restarts the game
- **THEN** its atlas entry or in-world landmark reflects the change without recompiling the engine
