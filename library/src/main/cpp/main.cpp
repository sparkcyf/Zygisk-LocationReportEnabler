#include <cstdio>
#include <cstdlib>
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <android/log.h>

#include "zygisk.hpp"

#include "logging.h"
#include "hook.h"
#include "android.h"
#include "config.h"

namespace mgl {
	
	class ZygiskModule : public zygisk::ModuleBase {
		public:
			void onLoad(zygisk::Api *api, JNIEnv *env) override {
				this->api = api;
				this->env = env;
			}
			void preAppSpecialize(zygisk::AppSpecializeArgs *args) override {
				// Use JNI to fetch our process name
				const char *process = env->GetStringUTFChars(args->nice_name, nullptr);
				// Use UNI to fetch our user dir
				const char *app_data_dir = env->GetStringUTFChars(args->app_data_dir, nullptr);
				
				if(process == nullptr || app_data_dir == nullptr) {
					return;
				}
				if(*process) {
					sscanf(process, "%s", saved_process_name);
				}
				
				if(*app_data_dir) {
					// /data/user/<user_id>/<package>
					if (sscanf(app_data_dir, "/data/%*[^/]/%d/%s", &saved_uid, saved_package_name) == 2)
						goto found;

					// /mnt/expand/<id>/user/<user_id>/<package>
					if (sscanf(app_data_dir, "/mnt/expand/%*[^/]/%*[^/]/%d/%s", &saved_uid, saved_package_name) == 2)
						goto found;

					// /data/data/<package>
					if (sscanf(app_data_dir, "/data/%*[^/]/%s", saved_package_name) == 1)
						goto found;

					// nothing found
					saved_package_name[0] = '\0';

					found:;
				}
				
				env->ReleaseStringUTFChars(args->nice_name, process);
				env->ReleaseStringUTFChars(args->app_data_dir, app_data_dir);
				
				int module_fd = api->getModuleDir();
				if(module_fd != -1)
					Config::Load(module_fd);
				
				preSpecialize();
			}

			void preServerSpecialize(zygisk::ServerSpecializeArgs *args) override {
				api->setOption(zygisk::DLCLOSE_MODULE_LIBRARY);
			}

			void postAppSpecialize(const zygisk::AppSpecializeArgs *args) override {
				LOGD("uid=%d, package=%s, process=%s", saved_uid, saved_package_name, saved_process_name);
				if(hook) {
					LOGI("Install hook... for %d:%s", saved_uid / 100000, saved_package_name);
					Hook::install();
				}
			}
		private:
			zygisk::Api *api = nullptr;
			JNIEnv *env = nullptr;
			bool hook = false;
			
			char saved_package_name[256] = {0};
			char saved_process_name[256] = {0};
			int saved_uid;

			void preSpecialize() {
				if(Config::Packages::Find(saved_package_name) == false) {
					LOGD("preSpecialize: package=[%s] not in config", saved_package_name);
					//hook = false;
					api->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);
					return;
				}
				Config::SetPackageName(saved_package_name);
				hook = true;
				api->setOption(zygisk::FORCE_DENYLIST_UNMOUNT);
			}
	};
}
REGISTER_ZYGISK_MODULE(mgl::ZygiskModule);
