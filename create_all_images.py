Import("env")

computer = "w11-p50"


#sub_boards = ["BOARD1", "BOARD2", "BOARD3", "BOARD4", "BOARD5", "BOARD6", "BOARD7", "BOARD8", "BOARD9", "BOARD10", "BOARD11", "BOARD12"]
sub_boards = ["BOARD1", "BOARD2", "BOARD3", "BOARD4", "BOARD5", "BOARD6", "BOARD7", "BOARD8", "BOARD9", "BOARD10", "BOARD11", "BOARD12"]
dom_board = "DOM"
#assert len(sub_boards) == 12


board = env.GetProjectOption("board")
environment = env.subst("$PIOENV")
program_path = env.subst("$PROG_PATH")
program_root = "/".join(program_path.split("/")[:program_path.split("/").index(".pio")])
print(program_root)

build_commands = []

current_board = dom_board
bootloader_offset = "0x1000 "
esptool_board = "esp32"
build_commands.append("pio run -e " + current_board + " --program-arg='-DBOARD" + str(len(sub_boards)+1) + "'")
build_commands.append("/usr/bin/python3 /home/seed/.platformio/packages/tool-esptoolpy/esptool.py --chip " + esptool_board + " merge_bin " + \
    bootloader_offset + program_root + "/.pio/build/" + current_board + "/bootloader.bin \
    0x8000 " + program_root + "/.pio/build/" + current_board + "/partitions.bin \
    0xe000 /home/seed/.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin \
    0x10000 " + program_root + "/.pio/build/" + current_board + "/firmware.bin -o " + program_root + "/dom.bin")
build_commands.append("sudo tailscale file cp " + program_root + "/dom.bin " + computer + ":")


for i in range(len(sub_boards)):
    current_board = sub_boards[i]
    if "c3" in current_board:
        esptool_board = "esp32c3"
    elif "s3" in current_board:
        esptool_board = "esp32s3"
    else:
        esptool_board = "esp32c6"
    
    if esptool_board == "esp32":
        bootloader_offset = "0x1000 "
    else: # all variants use 0x0000
        bootloader_offset = "0x0000 "
    # The reason we're using --program-arg='-DBOARD<1-12>' is because this is how the wifi channel is assigned
    build_commands.append("pio run -e " + current_board + " --program-arg='-DBOARD" + str(i+1) + "'")
    build_commands.append("/usr/bin/python3 /home/seed/.platformio/packages/tool-esptoolpy/esptool.py --chip " + esptool_board + " merge_bin " + \
        bootloader_offset + program_root + "/.pio/build/" + current_board + "/bootloader.bin \
        0x8000 " + program_root + "/.pio/build/" + current_board + "/partitions.bin \
        0xe000 /home/seed/.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin \
        0x10000 " + program_root + "/.pio/build/" + current_board + "/firmware.bin -o " + program_root + "/fullSub" + str(i+1) + ".bin")
    build_commands.append("sudo tailscale file cp " + program_root + "/fullSub" + str(i+1) + ".bin " + computer + ":")
    build_commands.append("mv " + program_root + "/.pio/build/" + current_board + "/firmware.bin sub" + str(i+1) + ".bin")
    build_commands.append("sudo tailscale file cp sub" + str(i+1) + ".bin " + computer + ":")
    

env.AddCustomTarget(
    name="CreateAllImages",
    dependencies=None,
    actions=build_commands,
    
    title="Create All Images",
    description="Generate images for each board, ready to flash with esptool.py at 0x0",
)
# "/usr/bin/python3" "/home/seed/.platformio/packages/tool-esptoolpy@src-8425bebf1f690909ef6a46ff6206d9fe/esptool.py" --chip esp32s3 --port "/dev/ttyACM0" --baud 460800 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 80m --flash_size detect 0x0000 /home/seed/PlatformIO/Projects/esp32s3nano-radar/.pio/build/esp32-s3-devkitc-1/bootloader.bin 0x8000 /home/seed/PlatformIO/Projects/esp32s3nano-radar/.pio/build/esp32-s3-devkitc-1/partitions.bin 0xe000 /home/seed/.platformio/packages/framework-arduinoespressif32@src-fa88b6a25f820284e99275b6f5b1fbc9/tools/partitions/boot_app0.bin 0x10000 .pio/build/esp32-s3-devkitc-1/firmware.binesp32-s3-matrix