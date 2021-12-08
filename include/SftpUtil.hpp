#ifndef __FILE_UPLOAD_H
#define __FILE_UPLOAD_H

#include <iostream>
#include <string>

#include <libssh2.h>
#include <libssh2_sftp.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>

#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>

using std::string;
using std::cout;
using std::endl;

class SftpUtil {
    string  address;
    string  username;
    string  password;
    string  targetPath;
    int port;

    int     sock;
    LIBSSH2_SESSION *session;

    public:
    SftpUtil(string _address, string _username, string _password, string _targetPath, int _port):
        address(_address), username(_username), password(_password), targetPath(_targetPath), port(_port) {
        }
    ~SftpUtil() {
        libssh2_session_free(session);
        libssh2_session_disconnect(session, "Normal Shutdown, Thank you for playing");
        close(sock);
        libssh2_exit();
	}

	bool initSession() {
		unsigned long hostaddr;
		int i, auth_pw = 1;
		struct sockaddr_in sin;
		const char *fingerprint;
		int rc;

		hostaddr = inet_addr(this->address.c_str());

		rc = libssh2_init(0);

		if(rc != 0) {
			fprintf(stderr, "libssh2 initialization failed (%d)\n", rc);
			return false;
		}

		/*
		 * The application code is responsible for creating the socket
		 * and establishing the connection
		 */
		sock = socket(AF_INET, SOCK_STREAM, 0);

		sin.sin_family = AF_INET;
		sin.sin_port = htons(this->port);
		sin.sin_addr.s_addr = hostaddr;
		if(connect(sock, (struct sockaddr*)(&sin),
					sizeof(struct sockaddr_in)) != 0) {
			fprintf(stderr, "failed to connect!\n");
			return false;
		}

		/* Create a session instance
		 */
		session = libssh2_session_init();

		if(!session)
			return false;

		/* Since we have set non-blocking, tell libssh2 we are blocking */
		libssh2_session_set_blocking(session, 1);


		/* ... start it up. This will trade welcome banners, exchange keys,
		 * and setup crypto, compression, and MAC layers
		 */
		rc = libssh2_session_handshake(session, sock);

		if(rc) {
			fprintf(stderr, "Failure establishing SSH session: %d\n", rc);
			return false;
		}

		/* At this point we havn't yet authenticated.  The first thing to do
		 * is check the hostkey's fingerprint against our known hosts Your app
		 * may have it hard coded, may go to a file, may present it to the
		 * user, that's your call
		 */
		fingerprint = libssh2_hostkey_hash(session, LIBSSH2_HOSTKEY_HASH_SHA1);

		fprintf(stderr, "Fingerprint: ");
		for(i = 0; i < 20; i++) {
			fprintf(stderr, "%02X ", (unsigned char)fingerprint[i]);
		}
		fprintf(stderr, "\n");

		if(auth_pw) {
			/* We could authenticate via password */
			if(libssh2_userauth_password(session, this->username.c_str(), this->password.c_str())) {

				fprintf(stderr, "Authentication by password failed.\n");
				return false;
			}
		}
		else {
			/* Or by public key */
			const char *pubkey = "/home/username/.ssh/id_rsa.pub";
			const char *privkey = "/home/username/.ssh/id_rsa.pub";
			if(libssh2_userauth_publickey_fromfile(session, this->username.c_str(),
						pubkey, privkey, this->password.c_str())) {
				fprintf(stderr, "\tAuthentication by public key failed\n");
				return false;
			}
		}

	}

	bool upload(string path, string localfile) {

		LIBSSH2_SFTP *sftp_session;
		LIBSSH2_SFTP_HANDLE *sftp_handle;
		char mem[1024*100];
		size_t nread;
		FILE *local;
		char *ptr;
		int rc;

		//const char *loclfile = "send/e176c2ec-dba8-53cc-958b-4befab9d1095_da9eb2d6-72e1-54af-92fc-75b1cb83bb58_1600147478.key";
		string localPath = path + localfile;
		local = fopen(localPath.c_str(), "rb");
		if(!local) {
			fprintf(stderr, "Can't open local file %s\n", localPath.c_str());
			return -1;
		}


		//fprintf(stderr, "libssh2_sftp_init()!\n");


		sftp_session = libssh2_sftp_init(session);


		if(!sftp_session) {
			fprintf(stderr, "Unable to init SFTP session\n");
			return false;
		}

		//fprintf(stdout, "[%s][%d]\n", __func__, __LINE__);
		//fprintf(stderr, "libssh2_sftp_open()!\n");

		/* Request a file via SFTP */
		string target = this->targetPath + localfile;
		sftp_handle =
			libssh2_sftp_open(sftp_session, target.c_str(),
					LIBSSH2_FXF_WRITE|LIBSSH2_FXF_CREAT|LIBSSH2_FXF_TRUNC,
					LIBSSH2_SFTP_S_IRUSR|LIBSSH2_SFTP_S_IWUSR|
					LIBSSH2_SFTP_S_IRGRP|LIBSSH2_SFTP_S_IROTH);

		if(!sftp_handle) {
			fprintf(stderr, "Unable to open file with SFTP\n");
			libssh2_sftp_shutdown(sftp_session);
			return false;
		}
		//fprintf(stderr, "libssh2_sftp_open() is done, now send data!\n");

		do {
			nread = fread(mem, 1, sizeof(mem), local);
			if(nread <= 0) {
				/* end of file */
				break;
			}
			ptr = mem;

			do {
				/* write data in a loop until we block */
				rc = libssh2_sftp_write(sftp_handle, ptr, nread);

				if(rc < 0)
					break;
				ptr += rc;
				nread -= rc;
			} while(nread);

		} while(rc > 0);

		libssh2_sftp_close(sftp_handle);
		libssh2_sftp_shutdown(sftp_session);

		return 0;

	}
};


#endif

