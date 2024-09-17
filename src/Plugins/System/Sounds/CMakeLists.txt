apx_plugin(QT Multimedia TextToSpeech)

apx_qrc(${MODULE} PREFIX audio BASE "${APX_RESOURCES_DIR}/audio" SRCS "**")

apx_install_res(${APX_RESOURCES_DIR} templates/speech.json)

