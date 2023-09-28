import subprocess

Import("env")

#def get_firmware_specifier_build_flag():
#    ret = subprocess.run(["git", "describe", "--tags"], stdout=subprocess.PIPE, text=True) #Uses only annotated tags
#    #ret = subprocess.run(["git", "describe", "--tags"], stdout=subprocess.PIPE, text=True) #Uses any tags
#    build_version = ret.stdout.strip()
#    build_flag = "-D AUTO_VERSION=\\\"" + build_version + "\\\""
#    print ("Firmware Revision: " + build_version)
#    return (build_flag)

def get_firmware_specifier_build_version():
    ret = subprocess.run(["git", "describe", "--dirty"], stdout=subprocess.PIPE, text=True) #Uses only annotated tags
    #ret = subprocess.run(["git", "describe", "--tags"], stdout=subprocess.PIPE, text=True) #Uses any tags
    build_version = ret.stdout.strip()
    return (build_version)

env.Replace(PROGNAME="GA_FUEL_TERMINAL-%s" % get_firmware_specifier_build_version())

env.Append(
    BUILD_FLAGS=["-D AUTO_VERSION=\\\"" + get_firmware_specifier_build_version() + "\\\""]
)