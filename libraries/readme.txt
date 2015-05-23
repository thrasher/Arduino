For information on installing libraries, see: http://arduino.cc/en/Guide/Libraries

Add submodules with:
git submodule add https://host.example.com/submodule.git
# module and .gitmodules are automatically staged for commit, so:
git commit

When cloning the project with submodules, for each submodule do:
git submodule init
# to initialize your local configuration file, and
git submodule update
# or, in one shot for all submodules:
git clone --recursive https://host.example.com/project.git
