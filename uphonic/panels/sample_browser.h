typedef void (*Uph_SampleLoadedFn)(bool loaded, Uph_ResourceIndex index, void *userdata);

bool uph_sample_browser_add_to_project(
	const Naui_Path path,
	Uph_SampleLoadedFn on_loaded,
	void *userdata
);
