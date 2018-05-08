# Translation set name
find_package(Gettext)
if(GETTEXT_FOUND)
	message( "msgfmt: ${GETTEXT_MSGFMT_EXECUTABLE}")
	message( "msgmerge: ${GETTEXT_MSGMERGE_EXECUTABLE}")
	set( POT_NAME "jwm" )
	set( POT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/${POT_NAME}.pot )
	set(MSGID_BUGS_ADDRESS "${PACKAGE_BUGREPORT}")
	set(COPYRIGHT_HOLDER "Free Software Foundation, Inc.")

	set( XGETTEXT_CMD "xgettext" )
	set(XGETTEXT_OPTIONS --default-domain="${PROJECT_NAME}" --directory="${CMAKE_CURRENT_SOURCE_DIR}" --keyword=_ --keyword=N_ --add-comments=TRANSLATORS: --files-from="${CMAKE_CURRENT_SOURCE_DIR}/POTFILES.in" --copyright-holder="${COPYRIGHT_HOLDER}" --msgid-bugs-address="${MSGID_BUGS_ADDRESS}" )

## TODO:
## package prefix GNU
#GETTEXT_VERSION_STRING

# make gettext options conform to version
	if("${GETTEXT_VERSION_STRING}" GREATER 0.16.1)
		message("Gettext version is ${GETTEXT_VERSION_STRING}")
		set(XGETTEXT_OPTIONS "${XGETTEXT_OPTIONS}" --package-name="$${package_prefix}jwm" --package-version='${VERSION}')
	endif()

	ADD_CUSTOM_COMMAND( OUTPUT ${POT_FILE}
		COMMAND ${XGETTEXT_CMD} ${XGETTEXT_OPTIONS} --output=${POT_FILE} ${POT_FILE_DEPENDS}
		WORKING_DIRECTORY "${JWM_SOURCE_DIR}"
		COMMENT "Extract translatable messages to ${POT_FILE}"
	)

	ADD_CUSTOM_TARGET( potfile
		COMMAND make translations
		COMMAND ${XGETTEXT_CMD} ${XGETTEXT_OPTIONS}
	)
endif()

