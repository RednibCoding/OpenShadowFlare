if(NOT EMSCRIPTEN)
    add_executable(SpriteDemo src/main.c)
    target_link_libraries(SpriteDemo PRIVATE RKC_UPDIB RKC_DBFCONTROL App)

    add_executable(CafDemo src/caf_main.c)
    target_link_libraries(CafDemo PRIVATE RKC_RPGSCRN_CAF RKC_UPDIB RKC_DBFCONTROL App)
endif()

add_library(game STATIC
    src/render.c
    src/paths.c
    src/sprites.c
    src/inventory.c
    src/script_bridge.c
    src/combat.c
    src/movement.c
    src/pathfind.c
    src/ground_scenario.c
)
target_include_directories(game PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(game PUBLIC
    RKC_RPGSCRN_GROUND RKC_RPGSCRN_OBJECT RKC_RPGSCRN_MCT RKC_RPG_SCRIPT
    RKC_RPG_SCRIPT_EXEC RKC_RPG_AICONTROL RKC_RPG_AI_EXEC RKC_RPG_ITEMDATA
    RKC_RPG_TABLE RKC_RPGSCRN_CAF RKC_UPDIB RKC_DBFCONTROL App RKC_DSOUND
)

add_executable(GroundDemo src/g_main.c)
target_link_libraries(GroundDemo PRIVATE game)

if(NOT EMSCRIPTEN)
    add_executable(MovementTraceTool src/movement_trace_tool.c)
    target_link_libraries(MovementTraceTool PRIVATE game)

    add_executable(WallCollisionProbe src/wall_collision_probe.c)
    target_link_libraries(WallCollisionProbe PRIVATE game)
endif()

if(EMSCRIPTEN)
    target_link_options(GroundDemo PRIVATE
        "-sMODULARIZE=1"
        "-sEXPORT_NAME=GroundDemoModule"
        "-sEXPORTED_RUNTIME_METHODS=[\"FS\",\"callMain\"]"
        "-sFORCE_FILESYSTEM=1"
        "-sALLOW_MEMORY_GROWTH=1"
        "-sINITIAL_MEMORY=536870912"
        "-sEXIT_RUNTIME=0"
        "-sENVIRONMENT=web"
        "-sSTACK_SIZE=1048576"
    )
    set(GROUNDDEMO_WEB_HTML ${CMAKE_CURRENT_SOURCE_DIR}/web/index.html)
    set(GROUNDDEMO_WEB_HTML_OUT ${CMAKE_CURRENT_BINARY_DIR}/index.html)
    set(GROUNDDEMO_WEB_WORKER ${CMAKE_CURRENT_SOURCE_DIR}/web/cross-origin-isolation-worker.js)
    set(GROUNDDEMO_WEB_WORKER_OUT ${CMAKE_CURRENT_BINARY_DIR}/cross-origin-isolation-worker.js)
    add_custom_command(
        OUTPUT ${GROUNDDEMO_WEB_HTML_OUT} ${GROUNDDEMO_WEB_WORKER_OUT}
        COMMAND ${CMAKE_COMMAND} -E copy ${GROUNDDEMO_WEB_HTML} ${GROUNDDEMO_WEB_HTML_OUT}
        COMMAND ${CMAKE_COMMAND} -E copy ${GROUNDDEMO_WEB_WORKER} ${GROUNDDEMO_WEB_WORKER_OUT}
        DEPENDS ${GROUNDDEMO_WEB_HTML} ${GROUNDDEMO_WEB_WORKER}
        COMMENT "Copying GroundDemo web shell")
    add_custom_target(GroundDemo_web_html ALL DEPENDS ${GROUNDDEMO_WEB_HTML_OUT} ${GROUNDDEMO_WEB_WORKER_OUT})
    add_dependencies(GroundDemo_web_html GroundDemo)
endif()
