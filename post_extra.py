Import("env")

print("Current CLI targets", COMMAND_LINE_TARGETS)
print("Current Build targets", BUILD_TARGETS)

def post_program_action(source, target, env):
    print("Program has been built!")
    program_path = target[0].get_abspath()
    print("Program path", program_path)
    # Use case: sign a firmware, do any manipulations with ELF, etc
    # env.Execute(f"sign --elf {program_path}")

env.AddPostAction("$PROGPATH", post_program_action)

# # Multiple actions
# env.AddCustomTarget(
#     name="pioenv",
#     dependencies=None,
#     actions=[
#         "pio --version",
#         "python --version"
#     ],
#     title="Upload via OTA",
#     description="Show PlatformIO Core and Python versions"
# )