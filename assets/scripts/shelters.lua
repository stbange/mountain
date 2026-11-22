-- Each destination selects a key from this loose catalogue. Changing a skin preserves the floor plan.
-- Event rows contain the control label, motion, tint and atmospheric extinction per metre.
-- Sightings contain the label, model, habitat, number of individuals and scale.
local function style(name,roof,base,colours,events,sights)
  return {name=name,roof=roof,base=base,wall=colours[1],frame=colours[2],
          cover=colours[3],accent=colours[4],events=events,sights=sights}
end
shelter_styles = {
  alpine=style("Timber cabin","gable","timber",{0xa2734d,0x69462f,0x705039,0xbc8958},{
    {"Thunderstorm","lightning",0x8395a6,.010},{"Snowstorm","snow",0xe2e9e5,.024},{"Heavy fog","mist",0xaebcbe,.065}},{
    {"a wolf pack","wolf","ground",3,1},{"a ridge giant","giant","ground",1,1}}),
  survey=style("Ranger cabin","gable","timber",{0xa37851,0x654938,0x495c55,0xcbad79},{
    {"Teton thunderstorm","lightning",0x798b9f,.012},{"Pass whiteout","snow",0xdde6eb,.041},{"Glacial lake fog","mist",0xb1c4ca,.055}},{
    {"a grizzly bear","bear","ground",1,1.05},{"a bull moose","moose","ground",1,1.1}}),
  coast=style("Cycladic boathouse","flat","masonry",{0xede6d7,0x316a87,0xe1d8c6,0x397c9e},{
    {"Meltemi spray","spray",0x98cbd3,.013},{"Breaking swells","surf",0xe6f2df,.003},{"Sea fog","mist",0xb9cdd1,.047}},{
    {"a barracuda shoal","barracuda","water",4,1.35},{"a waterspout","waterspout","water",1,1}}),
  dunes=style("Caravan tent","tent","pegs",{0x9a6d4f,0x4c434c,0x594958,0xd5ae76},{
    {"Sandstorm","sand",0xd6a26a,.041},{"Dust devils","vortex",0xd9b17b,.006},{"Heat shimmer","heat",0xf3d39c,.003}},{
    {"horned vipers","viper","ground",3,1},{"a collapsing dune","sandfall","ground",1,1}}),
  basalt=style("Turf shelter","turf","masonry",{0x68685e,0x4d4943,0x778052,0x9f9878},{
    {"Atlantic gale","spray",0x9aa9b0,.023},{"Sneaker waves","surf",0xd2ded8,.009},{"Sea fret","mist",0xa6b6b6,.059}},{
    {"an orca pod","orca","water",2,1},{"a basalt rockfall","rockfall","ground",1,1}}),
  mesa=style("Sandstone field hut","flat","masonry",{0xd1976e,0x685247,0xbd8a65,0x6f8e81},{
    {"Dry lightning","drylight",0xd8a77c,.009},{"Dust storm","sand",0xcd9166,.027},{"Canyon haze","mist",0xd5ac96,.029}},{
    {"a cougar","cougar","ground",1,1},{"a tumbling boulder field","rockfall","ground",1,1.4}}),
  karst=style("Bamboo stilt house","bamboo","stilts",{0xb69c6f,0x685843,0x56655b,0xa39a65},{
    {"Monsoon rain","rain",0x9cb9b3,.019},{"River surge","surf",0xa5c0a6,.012},{"Karst mist","mist",0xb7ccba,.042}},{
    {"bamboo pit vipers","green_viper","ground",2,.85},{"a raft of flood debris","driftwood","water",1,1}}),
  lunar=style("Lunar capsule","capsule","struts",{0xdfddd2,0x616a71,0xd5d5ca,0xb49a63},{
    {"Micrometeor impacts","impacts",0xe6e1d3,0},{"Ejecta fans","ejecta",0xb6b4af,0},{"Lofted regolith","dusthop",0xc5beb6,0}},{
    {"a runaway survey rover","rover","ground",1,1},{"a crater-wall landslide","rockfall","ground",1,1.6}}),
  prism=style("Prism capsule","capsule","struts",{0xabb8cb,0x504b72,0x8e9fbc,0x7dd3d0},{
    {"Prism lightning","drylight",0xb69de9,.009},{"Crystal shardfall","shards",0xa3eeee,.014},{"Violet veil","mist",0xa89ad4,.045}},{
    {"glass mantises","mantis","ground",2,1.1},{"a facet walker","facet","ground",1,1}}),
  fungal=style("Sporefilter capsule","capsule","struts",{0xa7b7a3,0x43585a,0x839787,0xb2daa4},{
    {"Spore squall","spores",0xb6d69b,.024},{"Lantern bloom","bloom",0xe2bd81,.009},{"Bioluminescent mist","mist",0x8cb7ae,.043}},{
    {"spore stalkers","stalker","ground",2,1},{"a lantern grazer","grazer","ground",1,1}}),
  reefs=style("Flotation capsule","capsule","struts",{0xe0d9bb,0x4e727c,0xa1bdbe,0xe0a299},{
    {"Gravity squall","updraft",0xa9dce0,.015},{"Sky tide","skytide",0xe2c4d9,.011},{"Reef mist","mist",0xafcbd1,.039}},{
    {"ribbon rays","ray","air",3,1},{"a sky leviathan","leviathan","air",1,1}}),
  cinder=style("Thermal capsule","capsule","struts",{0x978478,0x483e44,0x76666a,0xd58b57},{
    {"Emberfall","embers",0xf6ac67,.017},{"Volcanic lightning","drylight",0xe39b76,.022},{"Ash cloud","ash",0x94878a,.052}},{
    {"cinder hounds","hound","ground",3,1},{"a basalt colossus","colossus","ground",1,1}})
}
