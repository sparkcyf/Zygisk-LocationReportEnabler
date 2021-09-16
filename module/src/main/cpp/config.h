#pragma once

#include <string>

namespace Config {
	
	void SetPackageName(const char *name);
	std::string GetPackageName();

    struct Property {

        std::string name;
        std::string value;

        Property(const char *name, const char *value) : name(name), value(value) {}
    };

    void Load();

    namespace Properties {

        Property *Find(const char *name);
    }

    namespace Packages {

        bool Find(const char *name);
    }
}
