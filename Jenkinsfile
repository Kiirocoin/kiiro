pipeline {
    agent {
        docker {
            image 'kiirocoinorg/kiirocoin-builder-depends:latest'
            alwaysPull true
        }
    }
    environment {
        CCACHE_DIR = '/tmp/.ccache'
    }
    stages {
        stage('Build dependencies') {
            steps {
                sh 'git clean -d -f -f -q -x'
                dir('depends') {
                    sh 'make -j`nproc` HOST=x86_64-linux-gnu'
                }
            }
        }
        stage('Build') {
            steps {
                sh './autogen.sh'
                sh './configure --prefix=`pwd`/depends/x86_64-linux-gnu'
                sh 'make dist'
                sh 'mkdir -p dist'
                sh 'tar -C dist --strip-components=1 -xzf kiirocoin-*.tar.gz'
                dir('dist') {
                    sh './configure --prefix=`pwd`/../depends/x86_64-linux-gnu --enable-elysium --enable-tests --enable-crash-hooks'
                    sh 'make -j`nproc`'
                }
            }
        }
        stage('Test') {
            steps {
                dir('dist') {
                    sh 'make check'
                }
            }
        }
        stage('RPC Tests') {
            steps {
                dir('dist') {
                    sh 'qa/pull-tester/rpc-tests.py -extended'
                }
            }
        }
    }
}
