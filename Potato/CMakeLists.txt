cmake_minimum_required(VERSION 3.25.1)

ENABLE_CPP_MODULE()

add_library(PotatoModule STATIC 
	"PotatoIR.cpp"
	"PotatoReg.cpp"
	"PotatoSLRX.cpp"
	"PotatoEncode.cpp"
	#"PotatoEBNF.cpp"
	"PotatoDocument.cpp"
)

target_sources(
	PotatoModule PUBLIC
	FILE_SET cxx_modules TYPE CXX_MODULES
	FILES
		#"Potato.ixx"
		"PotatoMisc.ixx"
		"PotatoSTD.ixx"
		"PotatoTMP.ixx"
		"PotatoSmartPtr.ixx"
		"PotatoInterval.ixx"
		"PotatoIR.ixx"
		"PotatoSLRX.ixx"
		"PotatoReg.ixx"
		"PotatoFormat.ixx"
		"PotatoEncode.ixx"
		#"PotatoEBNF.ixx"
		"PotatoDocument.ixx"
)


target_compile_features(PotatoModule PUBLIC cxx_std_23)


if(MSVC)

	target_compile_options(PotatoModule PUBLIC "/experimental:module")

endif()