package=native_cmake
$(package)_version=3.14.7
$(package)_download_path=https://cmake.org/files/v3.14/
$(package)_file_name=cmake-$($(package)_version).tar.gz
$(package)_sha256_hash=9221993e0af3e6d10124d840ff24f5b2f3b884416fca04d3312cb0388dec1385

define $(package)_set_vars
  $(package)_config_opts=
endef

define $(package)_config_cmds
  ./bootstrap --prefix=$(build_prefix)
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef
