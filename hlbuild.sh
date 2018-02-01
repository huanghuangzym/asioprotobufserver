
export build_dir="/home/bol/.jenkins/jobs/bolservice/workspace"


export DEV_INCLUDES="-I${build_dir}/include/"
export DEV_LDADDS="-L${build_dir}/lib/"
export DEV_LIBSTATIC="${build_dir}/lib/"

svn up

make
