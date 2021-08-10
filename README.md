# OpenID-Connect Secure-gdal AFB-Extension

nav-gdal is an openid-connect/oAuth2 compliant extension to the application framework binder. It provides a secure filtering gdal for REST and websocket incoming requests.

On the external Internet interface is leverage OpenID user profile services to map incoming browser to profile and roles as defined by the identity authority. On the backend level (micro-service APIs) it respond to Cynagora privilege request for lower micro-service API to accept or deny a given request.

nav-gdal complies with any OpenID-connect identity public authority as github, google, microsoft, facebook, ... it also complies with internal authorities as Dex, Keycloak, Forgerock, ... For local authentication an optionnal PAM plugin is provided as sample local authentication template.

indentation: indent -ppi3 -i4 -nut -l160 -ip4 -as -slc -br -ce -di4 -brs *.c

Dependences
	afb-libafb
	pam-devel
	libcurl

makedir build & cd build
cmake ..
make

![gdal-biding-html5](docs/assets/afb-nav-gdal-archi.jpg)
