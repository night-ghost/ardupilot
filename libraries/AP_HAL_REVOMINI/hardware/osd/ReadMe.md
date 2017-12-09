Support for built-in MAX7456 for Omnibus FCs. Uses files from 
MinimOSD-Extra project (https://github.com/night-ghost/minimosd-extra) so
it should be downloaded and placed to AP_HAL_REVOMINI/support folder

Because Ardupolot is written on C++ and consists of objects with a hidden internal structure,
it is impossible to get the state of the flight controller from OSD driver. Therefore, 
the OSD works completely independently in own process, and communicates with the controller
using the Mavlink protocol - as well as external OSD does.
