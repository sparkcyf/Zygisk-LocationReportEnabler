#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <fcntl.h>
#include <jni.h>
#include <nativehelper/scoped_utf_chars.h>
#include <sys/system_properties.h>
#include <sys/socket.h>

#include "zygisk.hpp"

#include "logging.h"
#include "hook.h"
#include "android.h"
#include "config.h"

namespace mgl {
	namespace {
		ssize_t xsendmsg(int sockfd, const struct msghdr *msg, int flags) {
			int sent = sendmsg(sockfd, msg, flags);
			if (sent < 0) {
				PLOGE("sendmsg");
			}
			return sent;
		}

		ssize_t xrecvmsg(int sockfd, struct msghdr *msg, int flags) {
			int rec = recvmsg(sockfd, msg, flags);
			if (rec < 0) {
				PLOGE("recvmsg");
			}
			return rec;
		}

		// Read exact same size as count
		ssize_t xxread(int fd, void *buf, size_t count) {
			size_t read_sz = 0;
			ssize_t ret;
			do {
				ret = read(fd, (std::byte *) buf + read_sz, count - read_sz);
				if (ret < 0) {
					if (errno == EINTR)
						continue;
					PLOGE("read");
					return ret;
				}
				read_sz += ret;
			} while (read_sz != count && ret != 0);
			if (read_sz != count) {
				PLOGE("read (%zu != %zu)", count, read_sz);
			}
			return read_sz;
		}

		// Write exact same size as count
		ssize_t xwrite(int fd, const void *buf, size_t count) {
			size_t write_sz = 0;
			ssize_t ret;
			do {
				ret = write(fd, (std::byte *) buf + write_sz, count - write_sz);
				if (ret < 0) {
					if (errno == EINTR)
						continue;
					PLOGE("write");
					return ret;
				}
				write_sz += ret;
			} while (write_sz != count && ret != 0);
			if (write_sz != count) {
				PLOGE("write (%zu != %zu)", count, write_sz);
			}
			return write_sz;
		}

		int send_fds(int sockfd, void *cmsgbuf, size_t bufsz, const int *fds, int cnt) {
			iovec iov = {
					.iov_base = &cnt,
					.iov_len  = sizeof(cnt),
			};
			msghdr msg = {
					.msg_iov		= &iov,
					.msg_iovlen	 = 1,
			};

			if (cnt) {
				msg.msg_control = cmsgbuf;
				msg.msg_controllen = bufsz;
				cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
				cmsg->cmsg_len = CMSG_LEN(sizeof(int) * cnt);
				cmsg->cmsg_level = SOL_SOCKET;
				cmsg->cmsg_type = SCM_RIGHTS;

				memcpy(CMSG_DATA(cmsg), fds, sizeof(int) * cnt);
			}

			return xsendmsg(sockfd, &msg, 0);
		}

		int send_fd(int sockfd, int fd) {
			if (fd < 0) {
				return send_fds(sockfd, nullptr, 0, nullptr, 0);
			}
			char cmsgbuf[CMSG_SPACE(sizeof(int))];
			return send_fds(sockfd, cmsgbuf, sizeof(cmsgbuf), &fd, 1);
		}

		void *recv_fds(int sockfd, char *cmsgbuf, size_t bufsz, int cnt) {
			iovec iov = {
					.iov_base = &cnt,
					.iov_len  = sizeof(cnt),
			};
			msghdr msg = {
					.msg_iov		= &iov,
					.msg_iovlen	 	= 1,
					.msg_control	= cmsgbuf,
					.msg_controllen = bufsz
			};

			xrecvmsg(sockfd, &msg, MSG_WAITALL);
			cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);

			if (msg.msg_controllen != bufsz ||
				cmsg == nullptr ||
				cmsg->cmsg_len != CMSG_LEN(sizeof(int) * cnt) ||
				cmsg->cmsg_level != SOL_SOCKET ||
				cmsg->cmsg_type != SCM_RIGHTS) {
				return nullptr;
			}

			return CMSG_DATA(cmsg);
		}

		int recv_fd(int sockfd) {
			char cmsgbuf[CMSG_SPACE(sizeof(int))];

			void *data = recv_fds(sockfd, cmsgbuf, sizeof(cmsgbuf), 1);
			if (data == nullptr)
				return -1;

			int result;
			memcpy(&result, data, sizeof(int));
			return result;
		}

		int read_int(int fd) {
			int val;
			if (xxread(fd, &val, sizeof(val)) != sizeof(val))
				return -1;
			return val;
		}

		void write_int(int fd, int val) {
			if (fd < 0) return;
			xwrite(fd, &val, sizeof(val));
		}
	}
	
	class ZygiskModule : public zygisk::ModuleBase {
		public:
			void onLoad(zygisk::Api *api, JNIEnv *env) override {
				this->api = api;
				this->env = env;
			}
			void preAppSpecialize(zygisk::AppSpecializeArgs *args) override {
				appProcessPre(&args->uid, &args->nice_name, &args->app_data_dir);
				
				if (this->saved_package_name[0] != '\0') {
					int companion = this->api->connectCompanion();
					if (companion > 0) {
						auto module_dir = this->api->getModuleDir();
						send_fd(companion, module_dir);
						close(companion);
					} else {
						LOGE("failed to connect to companion");
					}
				}
			}
			void postAppSpecialize(const zygisk::AppSpecializeArgs *args) override {
				appProcessPost(this->saved_package_name, this->saved_uid);
			}
		private:
			zygisk::Api *api = nullptr;
			JNIEnv *env = nullptr;
			
			char saved_package_name[256] = {0};
			int saved_uid;
			#ifdef DEBUG
			char saved_process_name[256] = {0};
			#endif
			
			void appProcessPre(jint *uid, jstring *jNiceName, jstring *jAppDataDir) {
				saved_uid = *uid;
				
				#ifdef DEBUG
				//memset(saved_process_name, 0, 256);
				if (*jNiceName) {
					sprintf(saved_process_name, "%s", ScopedUtfChars(this->env, *jNiceName).c_str());
				}
				#endif
				
				//memset(saved_package_name, 0, 256);
				
				if (*jAppDataDir) {
					auto appDataDir = ScopedUtfChars(this->env, *jAppDataDir).c_str();
					int user = 0;
			
					// /data/user/<user_id>/<package>
					if (sscanf(appDataDir, "/data/%*[^/]/%d/%s", &user, saved_package_name) == 2)
						goto found;
			
					// /mnt/expand/<id>/user/<user_id>/<package>
					if (sscanf(appDataDir, "/mnt/expand/%*[^/]/%*[^/]/%d/%s", &user, saved_package_name) == 2)
						goto found;
			
					// /data/data/<package>
					if (sscanf(appDataDir, "/data/%*[^/]/%s", saved_package_name) == 1)
						goto found;
			
					// nothing found
					saved_package_name[0] = '\0';
					
					found:;
				}
			}
			void appProcessPost(const char *package_name, jint uid) {
					
				LOGD("uid=%d, package=%s, process=%s", uid, package_name, saved_process_name);
			
				Config::SetPackageName(package_name);
			
				if (Config::Packages::Find(package_name)) {
					LOGI("install hook for %d:%s", uid / 100000, package_name);
					Hook::install();
				} else {
					this->api->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);
				}
			}
	};
	
	static void CompanionEntry(int client) {
		auto module_dir = recv_fd(client);
		if (module_dir != -1){
			Config::Load(module_dir);
		} else {
			LOGE("failed to get ModuleDir");
		}
	}
}
REGISTER_ZYGISK_MODULE(mgl::ZygiskModule);
REGISTER_ZYGISK_COMPANION(mgl::CompanionEntry);