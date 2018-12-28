set(COMPONENT_SRCS
  "backend.c"
  "config.c"
  "console.c"
  "debug.c"
  "env.c"
  "experiments.c"
  "hsv2rgb.c"
  "jenkins.c"
  "json.c"
  "leds.c"
  "main.c"
  "mon.c"
  "status.c"
  "stuff.c"
  "wifi.c"
  )
set(COMPONENT_ADD_INCLUDEDIRS ".")

register_component()
