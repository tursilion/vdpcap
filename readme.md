20150403

This tool takes a VDP dump from a TMS9918A video chip (usually from an emulator) and lets you quickly recreate the original display and save it as an image.

VDP code from Classic99.

Two examples are provided - the master title page and Popeye.

The tool does not attempt to guess, but it makes it easy to click through the options and get the image narrowed down.

Examples:

- Run viewcapture.exe.
- Click Load Dump, select VDP-TITLE
- The title page will display, perhaps with the wrong screen color. The default settings for the application closely match the startup settings for the TI-99/4A.
- You can change the Screen Color drop down to Cyan to fix the image.

- Click Load Dump, select VDP-POPEYE
- The screen will appear quite scrambled, as POPEYE does not run in a common mode to the title page.
- POPEYE runs in Bitmap mode, so change the 'Default Settings' dropdown to 'Bitmap' to load common bitmap settings
- The screen will appear almost correct - with sprites appearing corrupted
- Select the Sprite Attributes drop down, and use the arrow keys to move up and down until you get sprites on the top, second and third levels - they will still be corrupted. This should end up at 1F00.
- Select the Sprite patterns drop down, and use the arrow keys to move up and down until the sprites look like Olive, Popeye and Brutus. This should happen at 3800.

This is the basic process. The more you know about what the image SHOULD be, the faster you can narrow it down. But with a little experience you can start with the Default settings to work out what mode looks closest, then work your way down the dropdowns and try to get each table correct.

There are additional options under the register numbers:

- Double sprites and Magnify sprites affect VDP register 1 and set the sprite MAGNIFY level.
- Sprite flicker turns the 4-sprite-per-line limit on and off
- No blanking disables the blanking bit in register 1
- No sprite disables the sprite layer
- No background disables the background layer
- 80 column hack may allow some F18A and some (16k!) 9938 80 column screens to display
