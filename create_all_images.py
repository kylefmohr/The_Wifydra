Import("env")

computer = "w11-p50"
# In order, where the first board is the top left, like:
# 1 2
# 3 4
# etc..
sub_boards = ["esp32-s3-matrix", "esp32-s3-matrix", "esp32-s3-xiao", "esp32-s3-xiao", "esp32-s3-zero", "esp32-s3-zero", "esp32-s3-tiny", "esp32-s3-tiny", "esp32-c3-supermini", "esp32-c3-supermini", "esp32-s3-zero", "esp32-s3-zero"]
dom_board = "esp32-s3-devkitc-1"
assert len(sub_boards) == 12


board = env.GetProjectOption("board")
environment = env.subst("$PIOENV")
program_path = env.subst("$PROG_PATH")
program_root = "/".join(program_path.split("/")[:program_path.split("/").index(".pio")])
print(program_root)
# A fatal error occurred: Please specify the chip argument (choose from esp8266, esp32, esp32s2, esp32s3beta2, esp32s3, esp32c3, esp32c6beta, esp32h2beta1, esp32h2beta2, esp32c2, esp32c6, esp32h2)
# (We need to get the name of the board that esptool.py uses)
esptool_board = "esp32"
if "esp32-s3" in board:
    esptool_board = "esp32s3"
elif "esp32-s2" in board:
    esptool_board = "esp32s2"
elif "esp32-c3" in board:
    esptool_board = "esp32c3"
elif "esp32-c6" in board:
    esptool_board = "esp32c6"
elif "esp32-h2" in board:
    esptool_board = "esp32h2"
elif "esp32-c2" in board:
    esptool_board = "esp32c2"

# The ESP32 uses a bootloader offset of 0x1000, all other variants use 0x0
if esptool_board == "esp32":
    bootloader_offset = "0x1000 "
else:
    bootloader_offset = "0x0000 "



build_commands = []
for i in range(len(sub_boards)):
    current_board = sub_boards[i]
    # The reason we're using --program-arg='-DBOARD<1-12>' is because this is how the wifi channel is assigned
    build_commands.append("pio run -e " + current_board + " --program-arg='-DBOARD" + str(i+1) + "'")
    build_commands.append("/usr/bin/python3 /home/seed/.platformio/packages/tool-esptoolpy/esptool.py --chip " + esptool_board + " merge_bin " + \
        bootloader_offset + program_root + "/.pio/build/" + current_board + "/bootloader.bin \
        0x8000 " + program_root + "/.pio/build/" + current_board + "/partitions.bin \
        0xe000 /home/seed/.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin \
        0x10000 " + program_root + "/.pio/build/" + current_board + "/firmware.bin -o " + program_root + "/sub" + str(i+1) + ".bin")
    build_commands.append("sudo tailscale file cp " + program_root + "/sub" + str(i+1) + ".bin " + computer + ":")
    
current_board = dom_board
build_commands.append("pio run -vvv -e " + current_board + " --program-arg='-DBOARD" + str(len(sub_boards)+1) + "'")
build_commands.append("/usr/bin/python3 /home/seed/.platformio/packages/tool-esptoolpy/esptool.py --chip " + esptool_board + " merge_bin " + \
    bootloader_offset + program_root + "/.pio/build/" + current_board + "/bootloader.bin \
    0x8000 " + program_root + "/.pio/build/" + current_board + "/partitions.bin \
    0xe000 /home/seed/.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin \
    0x10000 " + program_root + "/.pio/build/" + current_board + "/firmware.bin -o " + program_root + "/dom.bin")
build_commands.append("sudo tailscale file cp " + program_root + "/dom.bin " + computer + ":")

env.AddCustomTarget(
    name="CreateAllImages",
    dependencies=None,
    actions=build_commands,
    
    title="Create All Images",
    description="Generate images for each board, ready to flash with esptool.py at 0x0",
)
# "/usr/bin/python3" "/home/seed/.platformio/packages/tool-esptoolpy@src-8425bebf1f690909ef6a46ff6206d9fe/esptool.py" --chip esp32s3 --port "/dev/ttyACM0" --baud 460800 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 80m --flash_size detect 0x0000 /home/seed/PlatformIO/Projects/esp32s3nano-radar/.pio/build/esp32-s3-devkitc-1/bootloader.bin 0x8000 /home/seed/PlatformIO/Projects/esp32s3nano-radar/.pio/build/esp32-s3-devkitc-1/partitions.bin 0xe000 /home/seed/.platformio/packages/framework-arduinoespressif32@src-fa88b6a25f820284e99275b6f5b1fbc9/tools/partitions/boot_app0.bin 0x10000 .pio/build/esp32-s3-devkitc-1/firmware.bin