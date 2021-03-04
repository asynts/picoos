import invoke
import os
import tempfile

@invoke.task
def picoprobe(c):
    """
    This script connects to a picoprobe device that is used for debugging.
    Before using this script, the device needs to be plugged in.
    """

    c.sudo("openocd -f interface/picoprobe.cfg -f target/rp2040.cfg", pty=True)

@invoke.task
def debugger(c, gdb="/usr/local/arm-none-os-eabi/bin/arm-none-os-eabi-gdb"):
    """
    This script connects to the debugger interface exposed by picoprobe.
    Before using this script, picoprobe has to be run.
    """

    init_script = tempfile.NamedTemporaryFile(suffix=".gdb")

    init_script.write(b"""\
file Kernel.elf
target extended-remote localhost:3333

define rebuild
    shell ninja
    load
    monitor reset init
end
""")

    init_script.flush()

    c.run(f"{gdb} -q -x {init_script.name}", pty=True)

@invoke.task
def messages(c):
    """
    This script connects to the serial interface exposed by picoprobe.  Before
    using this script, picoprobe has to be run.
    """

    if not os.path.exists("/dev/ttyACM0"):
        print("Can not find serial device '/dev/ttyACM0'.")
        exit(1)

    c.sudo("stty -F /dev/ttyACM0 115200 igncr")
    c.sudo("cat /dev/ttyACM0", pty=True)
