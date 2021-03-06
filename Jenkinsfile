/* jenkins requirements:
 * - a Linux node with label: 'linux'
 * - a Windows node with label: 'windows'
 * - Jenkins email extended Plugin
 * - Jenkins sloccount Plugin
 * - Jenkins Warnings Next Generation Plugin
 * - Jenkins xUnit Plugin
 * - Both node must have meson and ninja installed
 * - Linux node must have CUnit installed
 * - Windows node must have VS2017 installed as we assume to have this compiler
 */

pipeline {
    agent none
    options {
        buildDiscarder(logRotator(numToKeepStr: '10'))
        timestamps()
    }

    stages {
        stage('Analyse') {
            agent any
            steps {
                sh 'cloc --by-file --xml --out=cloc.xml --vcs=git'
                sloccountPublish encoding: '', pattern: '**/cloc.xml'
            }
        }
        stage('Build') {
            parallel {
                stage('Linux build') {
                    agent {
                        label 'linux'
                    }
                    steps {
                        echo 'Building..'
                        /* output a make line so that warnings plugin understand
                         * relative path output by ninja */
                        echo "make[1]: Entering directory `build'"
                        sh 'meson build; ninja -C build/'
                        stash name: 'lib', includes: 'build/**'
                    }
                }
                stage('Windows build') {
                    agent {
                        label 'windows'
                    }
                    steps {
                        echo 'Building..'
                        /* we assume that we use vs2017 ce with standard path */
                        bat """
                            set path=%path:"=%
                            call "C:\\Program Files (x86)\\Microsoft Visual Studio\\2017\\Community\\VC\\Auxiliary\\Build\\vcvarsall.bat" x64
                            meson build
                            ninja -C build/
                        """
                    }
                }
                stage('Bare AVR build') {
                    agent {
                        label 'linux'
                    }
                    steps {
                        echo 'Building..'
                        echo "make[1]: Entering directory `build-avr'"
                        sh '''
                            meson build-avr --cross-file extras/linux-avr-atmega2560-cross.txt \
                                -Db_staticpic=false -Ddefault_library=static
                            ninja -C build-avr
                        '''
                    }
                }
            }

        }
        stage('Test') {
            agent {
                label 'linux'
            }
            options {
                skipDefaultCheckout()
            }
            steps {
                echo 'Testing..'
                unstash 'lib'
                /* ignore test error as they will be catched in post */
                sh 'SMP_TEST_AUTOMATED=1 ninja -C build/ test || true'
            }
            post {
                always {
                    xunit([CUnit(deleteOutputFiles: true, failIfNotNew: true,
                        pattern: 'build/test-all-*', skipNoTestFiles: false,
                        stopProcessingIfError: true)])
                }
            }
        }
    }
    post {
        always {
            node('master') {
                recordIssues enabledForFailure: true,
                    qualityGates: [[threshold: 1, type: 'TOTAL', unstable: true]],
                    tools: [gcc4(), msBuild()],
                    filters: [excludeMessage('.*Baud rate achieved is higher than allowed.*')]
            }
        }

        unstable {
            emailext to: '$DEFAULT_RECIPIENTS',
                recipientProviders: [culprits()],
                replyTo: '$DEFAULT_REPLYTO',
                subject: '$DEFAULT_SUBJECT',
                body: '$DEFAULT_CONTENT'
        }
        failure {
            emailext to: '$DEFAULT_RECIPIENTS',
                recipientProviders: [culprits()],
                replyTo: '$DEFAULT_REPLYTO',
                subject: '$DEFAULT_SUBJECT',
                body: '$DEFAULT_CONTENT'
        }
        fixed {
            emailext to: '$DEFAULT_RECIPIENTS',
                recipientProviders: [culprits()],
                replyTo: '$DEFAULT_REPLYTO',
                subject: '$DEFAULT_SUBJECT',
                body: '$DEFAULT_CONTENT'
        }
    }
}
