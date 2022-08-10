Import("env")

# Dump global construction environment (for debug purpose)
# print(env.Dump())

env.Replace(PROGNAME="firmware_%s" % env.GetProjectOption("custom_prog_version"))
# PROGPATH
# PROJECTSRC_DIR
# PROJECTDATA_DIR