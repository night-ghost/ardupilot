#include <sd/SD.h>

/*
  parse a parameter file line
 */
bool AP_Param::parse_param_line(char *line, char **vname, float &value)
{
    if (line[0] == '#') {
        return false;
    }
    char *saveptr = nullptr;
    char *pname = strtok_r(line, ", =\t", &saveptr);
    if (pname == nullptr) {
        return false;
    }
    if (strlen(pname) > AP_MAX_NAME_SIZE) {
        return false;
    }
    const char *value_s = strtok_r(nullptr, ", =\t", &saveptr);
    if (value_s == nullptr) {
        return false;
    }
    value = atof(value_s);
    *vname = pname;
    return true;
}

// increments num_defaults for each default found in filename
bool AP_Param::count_defaults_in_file(const char *filename, uint16_t &num_defaults, bool panic_on_error)
{
    File f = SD.open(filename, O_RDONLY);
    if (!f) {
        return false;
    }
    char line[100];

    /*
      work out how many parameter default structures to allocate
     */
    while (f.gets(line, sizeof(line)-1)) {
        char *pname;
        float value;
        if (!parse_param_line(line, &pname, value)) {
            continue;
        }
        enum ap_var_type var_type;
        if (!find(pname, &var_type)) {
            if (panic_on_error) {
                f.close();
                AP_HAL::panic("AP_Param: Invalid param in defaults file");
                return false;
            } else {
                continue;
            }
        }
        num_defaults++;
    }
    f.close();

    return true;
}

bool AP_Param::read_param_defaults_file(const char *filename)
{
    File f = SD.open(filename, O_RDONLY);
    if (!f) {
        AP_HAL::panic("AP_Param: Failed to re-open defaults file");
        return false;
    }

    uint16_t idx = 0;
    char line[100];
    while (f.gets(line, sizeof(line)-1)) {
        char *pname;
        float value;
        if (!parse_param_line(line, &pname, value)) {
            continue;
        }
        enum ap_var_type var_type;
        AP_Param *vp = find(pname, &var_type);
        if (!vp) {
            continue;
        }
        param_overrides[idx].object_ptr = vp;
        param_overrides[idx].value = value;
        idx++;
        if (!vp->configured_in_storage()) {
            vp->set_float(value, var_type);
        }
    }
    f.close();
    return true;
}

/*
  load a default set of parameters from a file
 */
bool AP_Param::load_defaults_file(const char *filename, bool panic_on_error)
{
    if (filename == nullptr) {
        return false;
    }

    char *mutable_filename = strdup(filename);
    if (mutable_filename == nullptr) {
        AP_HAL::panic("AP_Param: Failed to allocate mutable string");
    }

    uint16_t num_defaults = 0;
    char *saveptr = nullptr;
    for (char *pname = strtok_r(mutable_filename, ",", &saveptr);
         pname != nullptr;
         pname = strtok_r(nullptr, ",", &saveptr)) {
        if (!count_defaults_in_file(pname, num_defaults, panic_on_error)) {
            free(mutable_filename);
            return false;
        }
    }
    free(mutable_filename);

    if (param_overrides != nullptr) {
        free(param_overrides);
    }
    num_param_overrides = 0;

    param_overrides = new param_override[num_defaults];
    if (param_overrides == nullptr) {
        AP_HAL::panic("AP_Param: Failed to allocate overrides");
        return false;
    }

    saveptr = nullptr;
    mutable_filename = strdup(filename);
    if (mutable_filename == nullptr) {
        AP_HAL::panic("AP_Param: Failed to allocate mutable string");
    }
    for (char *pname = strtok_r(mutable_filename, ",", &saveptr);
         pname != nullptr;
         pname = strtok_r(nullptr, ",", &saveptr)) {
        if (!read_param_defaults_file(pname)) {
            free(mutable_filename);
            return false;
        }
    }
    free(mutable_filename);

    num_param_overrides = num_defaults;

    return true;
}

