

ifneq ($(shell),cmd)
obj-exist = $(subst ./,,$(shell find -name "*.o"))
obj-unused = $(filter-out $(objs)$(obj_S),$(obj-exist))
endif





deps = $(objs:.o=.d)

.PHONY:  all clean

ifeq ($(shell),cmd)

all: lib object 

lib: $(obj_S) $(objs) $(deps)
ifeq ($(CONFIG_LIB),y)
	$(AR) -r libout.a $(objs) $(obj_S)
endif
	
object: $(obj_S) $(objs) $(deps)
$(obj_S):%.o:%.S 
	@if exist $(subst /,\,$@) del $(subst /,\,$@)
	$(CC) $(FLAGS) $(includes) -D__ASSEMBLY__ $(DEFINS) -c $< -o $@ 
	
$(objs):%.o:%.c 
	@if exist $(subst /,\,$@) del $(subst /,\,$@)
	$(CC) $(FLAGS) $(includes) $(DEFINS) -c $< -o $@

	
depend:$(deps)
$(deps):%.d:%.c
	@$(CC) -MM $(includes) $< > $@.1
	@$(subst /,\,$(TOOLS))\funs.bat depend $@ $(dir $@) $(subst /,\,$@)

clean:
	-del $(subst /,\,$(objs))
	-del $(subst /,\,$(deps))



else

all: lib object remove-unused

lib: $(obj_S) $(objs) $(deps)
ifeq ($(CONFIG_LIB),y)
	$(AR) -r libout.a $(objs) $(obj_S)
endif
	
object: $(obj_S) $(objs) $(deps)
$(obj_S):%.o:%.S 
	$(CC) $(FLAGS) $(includes) $(DEFINS) $< -o $@ 
	
$(objs):%.o:%.c 
	$(CC) $(FLAGS) $(includes) $(DEFINS) $< -o $@ 
	
depend:$(deps)
$(deps):%.d:%.c 	
	@$(CC) -MM $(includes) $< | sed 's,\($(notdir $*)\)\.o[:]*,$(dir $@)\1.o $@ : ,g' >$@

remove-unused:
ifneq ($(obj-unused)x, x)
	rm $(obj-unused)
endif

clean:
	-rm $(obj_S) $(objs) $(deps)

endif


-include $(deps)

