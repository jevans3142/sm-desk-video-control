# ADC Theatre - SM Desk Video Switcher
## Configuration file format

An example configuration file for the SD card is included in this folder.
Any line beginning with // is regarded as a comment. 
The variable names must not be changed otherwise they will not be recognised. The equals sign also must be present. 

### Routing panel sources/destinations
Controls which source is routed to destination for each button and which output way on the router is used for each set of buttons.
Allowed values for sources: 1-40 = sources 1-40 on router, 99 = Show relay main/IR switched 'magic source'
Allowed values for destinations: 1-40 = destinations on router

| Variable name  | Format |
| ------------- | ------------- |
| routing_panel_sources[n] | Comma seperated list of numbers |
| routing_panel_destinations[n]  | Single number  |

### Show relay sources
Controls which sources are the 'main' and 'ir' cameras for the auto-switched outputs. Allowed values: 1-40 for sources are inputs 1-40 on router

| Variable name  | Format |
| ------------- | ------------- |
| show_relay_main_source | Single number |
| show_relay_ir_source  | Single number  |

### USB enables
Controls which sources when viewed on any screen should enable the respective USB channel. Allowed values: 1-40 for sources are inputs 1-40 on router

| Variable name  | Format |
| ------------- | ------------- |
| usb_source_a | Single number |
| usb_source_b  | Single number  |

### Main/IR switched outputs
Controls which destinations are affected by the main IR button on the SM desk ie. which outputs on the router are main/IR show relay outputs.
NB: Should not include the SM desk screen outputs above - only other screens.
Allowed values: list of destionations 1-40

| Variable name  | Format |
| ------------- | ------------- |
| show_relay_switched_destination_flags | Comma seperated list of numbers |

### Router properties

| Variable name  | Format |
| ------------- | ------------- |
| router_ip | IP address in x.x.x.x format, no quotes |
| router_port  | Single number  |