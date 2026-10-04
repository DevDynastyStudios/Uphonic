typedef struct
{
	uint64_t size;
	uint64_t hash;
}
Uph_FileSignature;
 
static inline bool uph_file_signature_equal(const Uph_FileSignature a, const Uph_FileSignature b)
{
	return a.size == b.size && a.hash == b.hash;
}

bool uph_file_signature(const Naui_Path path, Uph_FileSignature *out_signature);

bool uph_files_identical(const Naui_Path a, const Naui_Path b);
 