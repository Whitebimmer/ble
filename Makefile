export shell = $(SHELL)
# export shell = cmd
# export CPU = bt16
export CPU = br16

#------------------------Main tree - CPU   
-include Makefile.$(CPU)



