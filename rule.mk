 
objs_c = $(abspath $(objs))
objs_bs = $(abspath $(obj_S))
# objs_c = $(objs)
# objs_bs = $(obj_bs)
# objs_ls = $(obj_ls)
# objs_a =  $(LIB_NAME)_lib$(SUFFIX).a 
export OR32_NAME = $(PLATFORM)


deps = $(objs_c:.o=.d)
deps_bs = $(objs_bs:.o=.d)

#*************************************************************************
#
#						Windows
#
#*************************************************************************
ifeq ($(HOST_OS), windows) 

.PHONY: all clean test
# a: b c
	# @echo "-----"

# b:
	# @echo "*****"

# c:
	# @echo "^^^^^"
 
all: object 
ifeq ($(GEN_LIB),y)
	@if exist $(LIB_DIR) (rd /s/q $(LIB_DIR))
	@mkdir $(LIB_DIR)
	@if exist $(INCLUDE_LIB_PATH)\$(objs_a) del $(INCLUDE_LIB_PATH)\$(objs_a)
	$(V) $(AR) $(AR_ARGS) $(LIB_DIR)\$(objs_a) $(objs_c) $(objs_bs) 
	@copy $(LIB_DIR)\*.a $(INCLUDE_LIB_PATH)  /Y
else
	$(V) $(LD) $(LD_ARGS) $(APP_DIR)\$(OR32_NAME).or32 $(objs_c) $(objs_bs) $(LIBS) $(SYS_LIBS) $(LINKER)
	@$(APP_DIR)\download.bat $(OR32_NAME) $(OBJDUMP) $(OBJCOPY) $(SDK_TYPE)
endif
 
 
object: $(objs_bs) $(objs_c) $(deps)
$(objs_bs):%.o:%.S
	@if exist $(subst /,\,$@) del $(subst /,\,$@)
	$(V) $(CC)  $(SYS_INCLUDES) $(INCLUDES) -D__ASSEMBLY__ $(CC_ARGS) $(CC_ARG) -c $< -o $@
 
$(objs_c):%.o:%.c
	@if exist $(subst /,\,$@) del $(subst /,\,$@)
	@echo + CC $<
	$(V) $(CC)  $(SYS_INCLUDES) $(INCLUDES) $(CC_ARGS) $(CC_ARG) -c $< -o $@
 
$(deps):%.d:%.c
	$(V) $(CC) -MM $(SYS_INCLUDES) $(INCLUDES) $< > $@.1
	$(V) $(DEPENDS) $@ $(dir $@) $(subst /,\,$@)

$(deps_bs):%.d:%.S
	$(V) $(CC) -MM $(SYS_INCLUDES) $(INCLUDES) $< > $@.1
	$(V) $(DEPENDS) $@ $(dir $@) $(subst /,\,$@)

clean:
	@for /r %%i in (*.o) do del %%i
	@for /r %%i in (*.d) do del %%i
	@for /r %%i in (*.d.1) do del %%i
	@if exist $(APP_DIR)\$(OR32_NAME).or32 del $(APP_DIR)\$(OR32_NAME).or32

endif

#*************************************************************************
#
#						Linux
#
#*************************************************************************
ifeq ($(HOST_OS), linux) 
 
.PHONY: all clean test
 
all: object 
ifeq ($(GEN_LIB),y)
	@if [ ! -d $(LIB_DIR) ]; then mkdir $(LIB_DIR); fi
	@if [ -f $(LIB_DIR)/*.a ]; then rm -f $(LIB_DIR)/*.a; fi
	@if [ -f $(INCLUDE_LIB_PATH)/$(objs_a) ]; then rm -f $(INCLUDE_LIB_PATH)/$(objs_a); fi
	$(V) $(AR) $(AR_ARGS) $(LIB_DIR)/$(objs_a) $(objs_c) $(objs_ls) $(objs_bs) 
	@cp $(LIB_DIR)/*.a $(INCLUDE_LIB_PATH)
else
	$(V) $(LD) $(LD_ARGS) $(APP_DIR)/$(OR32_NAME).or32 $(objs_c) $(objs_ls) $(objs_bs) $(LIBS) $(SYS_LIBS) $(LINKER) --output-version-info $(APP_DIR)/ver.bin
	@cd $(APP_DIR) && $(SHELL) download.sh $(OR32_NAME)
endif
 
 
object: $(objs_ls) $(objs_bs) $(objs_c) $(deps)
$(objs_ls):%.o:%.s
	@if [ -f $@ ]; then rm $@; fi
	$(V) $(CC)  $(SYS_INCLUDES) $(INCLUDES) -D__ASSEMBLY__ $(CC_ARGS) -c $< -o $@
 
$(objs_bs):%.o:%.S
	@if [ -f $@ ]; then rm $@; fi
	$(V) $(CC)  $(SYS_INCLUDES) $(INCLUDES) -D__ASSEMBLY__ $(CC_ARGS) $(CC_ARG) -c $< -o $@
 
$(objs_c):%.o:%.c
	@if [ -f $@ ]; then rm $@; fi
	@echo + CC $<
	$(V) $(CC)  $(SYS_INCLUDES) $(INCLUDES) $(CC_ARGS) $(CC_ARG) -c $< -o $@
 
$(deps):%.d:%.c
	$(V) $(CC) -MM $(SYS_INCLUDES) $(INCLUDES) $< | sed 's,\($(notdir $*)\)\.o[:]*,$(dir $@)\1.o $@ : ,g' > $@

$(deps_bs):%.d:%.S
	$(V) $(CC) -MM $(SYS_INCLUDES) $(INCLUDES) $< | sed 's,\($(notdir $*)\)\.o[:]*,$(dir $@)\1.o $@ : ,g' > $@

$(deps_ls):%.d:%.s
	$(V) $(CC) -MM $(SYS_INCLUDES) $(INCLUDES) $< | sed 's,\($(notdir $*)\)\.o[:]*,$(dir $@)\1.o $@ : ,g' > $@
 
 
clean:
	@ -rm -f $(objs_c) $(deps) 
	@ -rm -f $(objs_bs) $(deps_bs)
	@ -rm -f $(objs_ls) $(deps_ls) 
	@ -rm -f $(APP_DIR)/$(OR32_NAME).or32

endif

ifneq ($(MAKECMDGOALS), clean)
-include $(deps)
# -include $(deps_ls)
# -include $(deps_bs)
endif
 
 
