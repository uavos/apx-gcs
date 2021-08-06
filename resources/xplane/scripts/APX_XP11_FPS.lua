
local start_time = os.clock()
local do_once = false

function apply_settings()
    if os.clock() > start_time and do_once == false then

	--Draw Aurora Borealis. 0 (off) or 1 (on)
	set( "sim/private/controls/reno/draw_aurora", 0.00)

	--Draw aircraft carriers and frigates. 0 (off) or 1 (on)
	set( "sim/private/controls/reno/draw_boats", 0.00)

	--Draw birds and deer in nice weather. 0 (off) or 1 (on)
	set( "sim/private/controls/reno/draw_deer_birds", 0.00)

	--Draw fires and baloons. 0 (off) or 1 (on)
	set( "sim/private/controls/reno/draw_fire_ball", 0.00)

	--Distance at which road traffic is visible. Default is 20000 meters. Recommend 7500.
	set( "sim/private/controls/cars/lod_min", 0.00)

	--Distance at which static plans are visible. Default is 9260 meters. Recommend 3000.
	set( "sim/private/controls/park/static_plane_build_dis", 0.00)

	--Runways follow terrain contours. 0 (off) or 1 (on)
	set( "sim/private/controls/reno/sloped_runways", 0.00)

	--Atmospheric scattering. 0 (off) or 1 (on)
	set( "sim/private/controls/reno/draw_scattering", 0.00)

	--Draw volumetric fog. 0 (off) or 1 (on)
	set( "sim/private/controls/reno/draw_volume_fog01", 0.00)

	--Draw per pixel lighting. 0 (off) or 1 (on). Change both of the settings below to be the same.
	set( "sim/private/controls/reno/draw_per_pix_liting", 0.00)

	--3D Object Density. 0 (None) to 6 (Extreme)
	set( "sim/private/controls/reno/draw_objs_06", 0.00)

	--Road traffic density. 0 (off) to 5 (very dense)
	set( "sim/private/controls/reno/draw_cars_05", 0.00)

	--Road Density. 0 (None) to 3 (Extreme)
	set( "sim/private/controls/reno/draw_vecs_03", 0.00)

	--Runway and taxiway line smoothness and 3D runway/taxiway lighting. 0 (Low) to 3 (Extreme)
	set( "sim/private/controls/reno/draw_detail_apt_03", 0.00)

	--Forest density. 0 (Low) to 5 (Extreme)
	set( "sim/private/controls/reno/draw_for_05", 0.00)

	--Forest inner ring density. 0 (0%) to 1 (100%)
	set( "sim/private/controls/forest/inn_ring_density", 0.00)

	--Forest mid ring density. 0 (0%) to 1 (100%)
	set( "sim/private/controls/forest/mid_ring_density", 0.00)

	--Forest outer ring density. 0 (0%) to 1 (100%)
	set( "sim/private/controls/forest/out_ring_density", 0.00)

	--Fade start rate. 0.75 (low setting) to 0.6 (high setting)
	set( "sim/private/controls/terrain/fade_start_rat", 0.75)

	--Tile LOD bias. 0.72 (low setting) to 1.0 (high setting)
	set( "sim/private/controls/ag/tile_lod_bias", 0.72)

	--Composite far distance bias. 0.72 (low setting) to 1.0 (high setting)
	set( "sim/private/controls/terrain/composite_far_dist_bias", 0.72)

	--Water reflection detail. 0 (None) to 5 (Complete)
	set( "sim/private/controls/reno/draw_reflect_water05", 0.00)

	--Compress textures to save VRAM. 0 (off) or 1 (on)
	set( "sim/private/controls/reno/comp_texes", 0.00)

	--Use bump map textures. 0 (off) or 1 (on)
	set( "sim/private/controls/reno/use_bump_maps", 0.00)

	--Use detail (aka gritty) textures or decals. 0 (off) or 1 (on)
	set( "sim/private/controls/reno/use_detail_textures", 0.00)

	--Ambient Occlusion. 0 (off) or 1 (on)
	set( "sim/private/controls/ssao/enable", 0.00)

	--Static plane density. 1 (low) to 6 (high). Note this does not affect static planes manually placed by scenery designer, just the ones that automatically appear and start areas.
	set( "sim/private/controls/park/static_plane_density", 0.00)

	--Cascading Shadow Maps Exterior Quality (Higher numbers reduce jagged edges of shadows). Known valid values are 1 or 2 for XP11.  XP10 used values of 0-5.
	set( "sim/private/controls/shadow/csm_split_exterior", 1.00)

	--Shadow fade distance. XP11 uses values ranging from 500 to 1500.  XP10 used values ranging from 500 to 6000.
	-- set( "sim/private/controls/shadow/csm/far_limit", 500.00)

	--Shadow texture size??? XP11 uses either 2048 or 4096. This setting was not used in XP10
	set( "sim/private/controls/fbo/shadow_cam_size", 2048.00)

	--The following values adjust the maximum distance ground scenery is visible.  Default is 10000.  Larger values seem to have no affect, but you might try lower values if you have a low end system.
	set( "sim/private/controls/skyc/max_dsf_vis_ever", 2000.00)

	--The following values adjust fading of ground textures in the distance.  Default is 0.90 in XP10 (0.75 in XP 11).  Higher values have no effect.  But you might try lower values (0.10 for example) if you want the ground scenery to fade to brown more gradually in the distance.
	set( "sim/private/controls/skyc/dsf_fade_ratio", 0.90)

	--The following enables reflective water. Default is 1.00 (on)
	set( "sim/private/controls/caps/use_reflective_water", 0.00)
	
	-- Draw fft water
	set( "sim/private/controls/reno/draw_fft_water", 0.00)

	-- Cockpit near Proxy
	set( "sim/private/controls/shadow/cockpit_near_proxy", 3.0)

	-- Cockpit near adjust
	set( "sim/private/controls/shadow/cockpit_near_adjust", 1.0)
	

	-- some other props found ever
	set( "sim/private/controls/skyc/max_dsf_vis_ever", 2000)
	set( "sim/private/controls/perf/disable_shadow_prep",0)
	set( "sim/private/controls/shadow/scenery_shadows",0)
	set( "sim/private/controls/clouds/shadow_size", 2048)
	set( "sim/private/controls/clouds/plot_radius", 2.5)
	set( "sim/private/controls/reno/draw_reflect_water05", 0.00)
	set( "sim/private/controls/reno/draw_reflect_water05", 0.00)
	set( "sim/private/controls/ssao/enable", 0.00)

	set( "sim/graphics/view/world_render_type", 0)
	set( "sim/graphics/settings/rendering_res", 1)
	set( "sim/graphics/view/visibility_terrain_m", 0.0)
	set( "sim/private/controls/reno/draw_HDR", 0)


	do_once=true
    end
end

do_often("apply_settings()")

