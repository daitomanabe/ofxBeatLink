################################################################################
# CONFIGURE PROJECT MAKEFILE (optional)
################################################################################

# Enable VirtualCdj and TCP features by removing the disable flags
# Override the default flags from addon_config.mk
PROJECT_CFLAGS = -DASIO_STANDALONE
