bin_PROGRAMS = pmu

pmu_CFLAGS = $(PMU_CFLAGS)
pmu_LDADD = $(PMU_LIBS)
pmu_SOURCES = \
	main.c 			\
	pmu-app.h 		\
	pmu-app.c 		\
	pmu-window.h 		\
	pmu-window.c 		\
	pmu-setup-window.h 		\
	pmu-setup-window.c 		\
	pmu-details.h 		\
	pmu-details.c 		\
	c37-common.h 		\
	c37-common.c 		\
	resources.c

BUILT_SOURCES = \
	resources.c

gsettings_SCHEMAS = $(srcdir)/resources/org.sadiqpk.pmu.gschema.xml

@GSETTINGS_RULES@

resource_files = $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir)/resources --generate-dependencies $(srcdir)/resources/pmu.gresource.xml)

resources.c: resources/pmu.gresource.xml $(resource_files)
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) $(srcdir)/resources/pmu.gresource.xml \
                --target=$@ --sourcedir=$(srcdir)/resources --generate-source

EXTRA_DIST = \
	$(resource_files) \
	$(gsettings_SCHEMAS)

-include $(top_srcdir)/git.mk
