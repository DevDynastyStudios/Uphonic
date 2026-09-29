void uph_action_initialize()
{
	naui_register_action(UPH_ACTION_PATTERN_CREATE,
	{
		.execute = _uph_action_pattern_create_execute,
		.undo = _uph_action_pattern_create_undo,
		.redo = _uph_action_pattern_create_redo
	});

	naui_register_action(UPH_ACTION_TRACK_CREATE,
	{
		.execute = _uph_action_track_create_execute,
		.undo = _uph_action_track_create_undo,
		.redo = _uph_action_track_create_redo
	});

	// naui_register_action(UPH_ACTION_TRACK_RENAME,
	// {
	// 	.execute = _uph_action_track_rename_execute,
	// 	.undo = _uph_action_track_rename_undo,
	// 	.redo = _uph_action_track_rename_redo
	// });

	naui_register_action(UPH_ACTION_AUTOMATION_CREATE,
	{
		.execute = _uph_action_automation_create_execute,
		.undo = _uph_action_automation_create_undo,
		.redo = _uph_action_automation_create_redo
	});
}

void uph_action_shutdown()
{

}