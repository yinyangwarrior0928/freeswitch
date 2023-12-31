

#include <switch.h>
#include <switch_config.h>

SWITCH_DECLARE(int) switch_config_open_file(switch_config_t *cfg, char *file_path)
{
	FILE *f;
	char *path = NULL;
	char path_buf[1024];

	if (switch_is_file_path(file_path)) {
		path = file_path;
	} else {
		switch_snprintf(path_buf, sizeof(path_buf), "%s%s%s", SWITCH_GLOBAL_dirs.conf_dir, SWITCH_PATH_SEPARATOR, file_path);
		path = path_buf;
	}

	if (!path) {
		return 0;
	}

	memset(cfg, 0, sizeof(*cfg));
	cfg->lockto = -1;

	if (!(f = fopen(path, "r"))) {
		if (!switch_is_file_path(file_path)) {
			int last = -1;
			char *var, *val;

			switch_snprintf(path_buf, sizeof(path_buf), "%s%sfreeswitch.conf", SWITCH_GLOBAL_dirs.conf_dir, SWITCH_PATH_SEPARATOR);
			path = path_buf;

			if ((f = fopen(path, "r")) == 0) {
				return 0;
			}

			cfg->file = f;
			switch_set_string(cfg->path, path);

			while (switch_config_next_pair(cfg, &var, &val)) {
				if (file_path && (cfg->sectno != last) && !strcmp(cfg->section, file_path)) {
					cfg->lockto = cfg->sectno;
					return 1;
				}
			}

			switch_config_close_file(cfg);
			memset(cfg, 0, sizeof(*cfg));
			return 0;
		}

		return 0;
	} else {
		cfg->file = f;
		switch_set_string(cfg->path, path);
		return 1;
	}
}

SWITCH_DECLARE(void) switch_config_close_file(switch_config_t *cfg)
{

	if (cfg->file) {
		fclose(cfg->file);
	}

	memset(cfg, 0, sizeof(*cfg));
}

SWITCH_DECLARE(int) switch_config_next_pair(switch_config_t *cfg, char **var, char **val)
{
	int ret = 0;
	char *p, *end;

	*var = *val = NULL;

	if ( !cfg->path || (cfg->path && cfg->path[0] != '\0' )) {
		return 0;
	}

	for (;;) {
		cfg->lineno++;

		if (!fgets(cfg->buf, sizeof(cfg->buf), cfg->file)) {
			ret = 0;
			break;
		}
		*var = cfg->buf;

		if (**var == '[' && (end = strchr(*var, ']')) != 0) {
			*end = '\0';
			(*var)++;
			if (**var == '+') {
				(*var)++;
				switch_copy_string(cfg->section, *var, sizeof(cfg->section));
				cfg->sectno++;

				if (cfg->lockto > -1 && cfg->sectno != cfg->lockto) {
					break;
				}
				cfg->catno = 0;
				cfg->lineno = 0;
				*var = "";
				*val = "";
				return 1;
			} else {
				switch_copy_string(cfg->category, *var, sizeof(cfg->category));
				cfg->catno++;
			}
			continue;
		}

		if (**var == '#' || **var == ';' || **var == '\n' || **var == '\r') {
			continue;
		}

		if (!strncmp(*var, "__END__", 7)) {
			break;
		}

		if ((end = strchr(*var, '#')) != 0 || (end = strchr(*var, ';')) != 0) {
			*end = '\0';
			end--;
		} else if ((end = strchr(*var, '\n')) != 0) {
			if (*(end - 1) == '\r') {
				end--;
			}
			*end = '\0';
		}

		p = *var;
		while ((*p == ' ' || *p == '\t') && p != end) {
			*p = '\0';
			p++;
		}
		*var = p;

		if ((*val = strchr(*var, '=')) == 0) {
			ret = -1;
			/* log_printf(0, server.log, "Invalid syntax on %s: line %d\n", cfg->path, cfg->lineno); */
			continue;
		} else {
			p = *val - 1;
			*(*val) = '\0';
			(*val)++;
			if (*(*val) == '>') {
				*(*val) = '\0';
				(*val)++;
			}

			while ((*p == ' ' || *p == '\t') && p != *var) {
				*p = '\0';
				p--;
			}

			p = *val;
			while ((*p == ' ' || *p == '\t') && p != end) {
				*p = '\0';
				p++;
			}
			*val = p;
			ret = 1;
			break;
		}
	}

	return ret;
}

/* For Emacs:
 * Local Variables:
 * mode:c
 * indent-tabs-mode:t
 * tab-width:4
 * c-basic-offset:4
 * End:
 * For VIM:
 * vim:set softtabstop=4 shiftwidth=4 tabstop=4 noet:
 */
