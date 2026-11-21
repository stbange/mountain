-- Loose game content. Reload after editing. Colours are 0xRRGGBB.
return {
  seed = 4812, hour = 16.65, season = 1, weather = 0,
  survey = { seed = 1872, name = "Taggart Lake", region = "Wyoming", file = "terrain/taggart.mht",
             scale = 5, surface = 2101.36, bearing = -0.82 },
  clock_rate = 0.006, season_interval = 480, weather_interval = 330, pace = 5.3,
  relief = 1.0, water = 0,
  trails = { "Alder Pass", "Stillwater Fell", "Juniper Notch", "Foxglove Vale",
             "Larkspur Ridge", "Cloudberry Tarn", "Silver Fir Pass", "Wren Peak" },
  seasons = { "Spring", "Summer", "Autumn", "Winter" },
  skies = { "Clear", "Cloudy", "Rain", "Mist" },
  destination_file = "destinations.lua",
  fauna_script = "wildlife.lua",
  fauna_names = { "Mule deer", "Red fox", "Snowshoe hare", "Yellow-bellied marmot", "Golden eagle" },
  -- Subalpine fir, spruce, larch and aspen.
  tree_shapes = {
    {form="whorled", deciduous=false, first_bough=4, bough_variation=3, width=1, droop=1.05, foliage=1},
    {form="whorled", deciduous=false, first_bough=4, bough_variation=3, width=1, droop=1.72, foliage=1},
    {form="whorled", deciduous=true, first_bough=9, bough_variation=0, width=1, droop=1.05, foliage=.79},
    {form="forked", deciduous=true, first_bough=14, bough_variation=0, width=1, droop=0, foliage=1},
  },
  needles = {0x436445,0x3d6356,0x749253,0x799951},
  bark = {0x785d44,0x695747,0x816a49,0xc0b99e},
  autumn_canopy = {0x476948,0x46695a,0xd5ab48,0xe1ad49},
  -- Atlas tiles for flowering patches, in season order.
  field_tiles = { {12, 8}, {4, 13}, {14, 14}, {5, 5} },
  winter_precipitation = "Snow",
  meadow = { 0x8caa64, 0xa8ad69, 0xc39a52, 0xd4e0dc },
  forest = { 0x506e52, 0x567253, 0x7b7851, 0x899e98 }
}
