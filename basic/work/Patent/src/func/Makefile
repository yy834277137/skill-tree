#func/下的库会依赖func_baseLib/下的库，但func/下的库相互之间没有依赖关系，func_baseLib下的库相互之间也没有依赖关系
OUT_FUNC_LIB_DIR := OUT_FUNC_LIBS
export OUT_FUNC_LIB_DIR

all_libs:
	@mkdir -p $(OUT_FUNC_LIB_DIR)
	+@make -C func_baseLib/iconv            module=iconv
	+@make -C func_baseLib/freetype         module=freetype
	+@make -C func_baseLib/harfbuzz         module=harfbuzz
	+@make -C func_baseLib/pack_lib         module=pack_lib
	+@make -C func_baseLib/unpack_lib       module=unpack_lib
	+@make -C func_baseLib/vca_system       module=vca_system
	+@make -C func_baseLib/mcu              module=mcu
	+@make -C audio                         module=audio 
	+@make -C captchip                      module=captchip
	+@make -C dispchip                      module=dispchip
	+@make -C gif                           module=gif
	+@make -C jpeg                          module=jpeg
	+@make -C jpeg_soft                     module=jpeg_soft
	+@make -C hik_downsmp                   module=hik_downsmp
	+@make -C link                          module=link
	+@make -C osd                           module=osd
	+@make -C svp_dsp                       module=svp_dsp
#	+@make -C vda                           module=vda
	+@make -C venc                          module=venc
	+@make -C vgs                           module=vgs


clean:

	@make clean -C func_baseLib/iconv         module=iconv
	@make clean -C func_baseLib/freetype      module=freetype
	@make clean -C func_baseLib/harfbuzz      module=harfbuzz
	@make clean -C func_baseLib/pack_lib      module=pack_lib
	@make clean -C func_baseLib/unpack_lib    module=unpack_lib
	@make clean -C func_baseLib/vca_system    module=vca_system
	@make clean -C func_baseLib/mcu           module=mcu
	@make clean -C audio                      module=audio 
	@make clean -C captchip                   module=captchip
	@make clean -C dispchip                   module=dispchip
	@make clean -C gif                        module=gif
	@make clean -C jpeg                       module=jpeg
	@make clean -C jpeg_soft                  module=jpeg_soft
	@make clean -C hik_downsmp                module=hik_downsmp
	@make clean -C link                       module=link
	@make clean -C osd                        module=osd
	@make clean -C svp_dsp                    module=svp_dsp
#	@make clean -C vda                        module=vda
	@make clean -C venc                       module=venc
	@make clean -C vgs                        module=vgs
	@rm -rf $(OUT_FUNC_LIB_DIR)
