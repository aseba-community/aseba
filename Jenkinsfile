#!groovy

// Jenkinsfile for compiling, testing, and packaging Aseba.
// Requires CMake plugin from https://github.com/davidjsherman/aseba-jenkins.git in global library.

// Ideally we should unarchive Dashel and Enki libraries that were previously compiled by other
// Jenkins jobs, but for now we conservatively recompile them.

pipeline {
	agent any

	// Jenkins will prompt for parameters when a branch is build manually
	// but will use default parameters when the entire project is built.
	parameters {
		stringParam(defaultValue: 'master', description: 'Dashel branch', name: 'branch_dashel')
		stringParam(defaultValue: 'master', description: 'Enki branch', name: 'branch_enki')
	}
	
	stages {
		stage('Prepare') {
			steps {
				echo "branch_dashel=${env.branch_dashel}"
				echo "branch_enki=${env.branch_enki}"

				// Dashel and Enki will be checked out into the externals/ directory.
				sh 'mkdir -p externals'
				dir('aseba') {
					checkout scm
					sh 'git submodule update --init'
				}
				dir('externals/dashel') {
					git branch: "${env.branch_dashel}", url: 'https://github.com/aseba-community/dashel.git'
				}
				dir('externals/enki') {
					git branch: "${env.branch_enki}", url: 'https://github.com/enki-community/enki.git'
				}
				stash excludes: '.git', includes: 'aseba/**,externals/**', name: 'source'
			}
		}
		// Everything will be built in the build/ directory.
		// Everything will be installed in the dist/ directory.
		stage('Dashel') {
			steps {
				parallel (
					"debian" : {
						node('debian') {
							unstash 'source'
							CMake([sourceDir: pwd()+'/externals/dashel', label: 'debian', preloadScript: 'set -x',
								buildDir: pwd()+'/build/dashel/debian'])
							stash includes: 'dist/**', name: 'dist-dashel-debian'
						}
					},
					"macos" : {
						node('macos') {
							// sh 'rm -rf build/* dist/*'
							unstash 'source'
							CMake([sourceDir: pwd()+'/externals/dashel', label: 'macos', preloadScript: 'set -x',
								buildDir: pwd()+'/build/dashel/macos'])
							stash includes: 'dist/**', name: 'dist-dashel-macos'
						}
					},
					"windows" : {
						node('windows') {
							// sh 'rm -rf build/* dist/*'
							unstash 'source'
							CMake([sourceDir: pwd()+'/externals/dashel', label: 'windows', preloadScript: 'set -x',
								buildDir: pwd()+'/build/dashel/windows'])
							stash includes: 'dist/**', name: 'dist-dashel-windows'
						}
					}
				)
			}
		}
		stage('Enki') {
			steps {
				parallel (
					"debian" : {
						node('debian') {
							unstash 'source'
							script {
								env.debian_python = sh ( script: '''
									python -c "import sys; print 'lib/python'+str(sys.version_info[0])+'.'+str(sys.version_info[1])+'/dist-packages'"
								''', returnStdout: true).trim()
							}
							CMake([sourceDir: pwd()+'/externals/enki', label: 'debian', preloadScript: 'set -x',
								getCmakeArgs: "-DPYTHON_CUSTOM_TARGET:PATH=${env.debian_python}",
								buildDir: pwd()+'/build/enki/debian'])
							stash includes: 'dist/**', name: 'dist-enki-debian'
						}
					},
					"macos" : {
						node('macos') {
							unstash 'source'
							CMake([sourceDir: pwd()+'/externals/enki', label: 'macos', preloadScript: 'set -x',
								buildDir: pwd()+'/build/enki/macos'])
							stash includes: 'dist/**', name: 'dist-enki-macos'
						}
					},
					"windows" : {
						node('windows') {
							unstash 'source'
							CMake([sourceDir: pwd()+'/externals/enki', label: 'windows', preloadScript: 'set -x',
								buildDir: pwd()+'/build/enki/windows'])
							stash includes: 'dist/**', name: 'dist-enki-windows'
						}
					}
				)
			}
		}
		stage('Compile') {
			steps {
				parallel (
					"debian" : {
						node('debian') {
							unstash 'dist-dashel-debian'
							unstash 'dist-enki-debian'
							script {
								env.debian_dashel_DIR = sh ( script: 'dirname $(find $PWD/dist/debian -name dashelConfig.cmake | head -1)', returnStdout: true).trim()
								env.debian_enki_DIR = sh ( script: 'dirname $(find $PWD/dist/debian -name enkiConfig.cmake | head -1)', returnStdout: true).trim()
							}
							unstash 'source'
							CMake([sourceDir: pwd()+'/aseba', label: 'debian', preloadScript: 'set -x',
								envs: [ "dashel_DIR=${env.debian_dashel_DIR}", "enki_DIR=${env.debian_enki_DIR}" ] ])
							stash includes: 'dist/**', name: 'dist-aseba-debian'
							stash includes: 'build/**', name: 'build-aseba-debian'
						}
					},
					"macos" : {
						node('macos') {
							unstash 'dist-dashel-macos'
							unstash 'dist-enki-macos'
							script {
								env.macos_dashel_DIR = sh ( script: 'dirname $(find $PWD/dist/macos -name dashelConfig.cmake | head -1)', returnStdout: true).trim()
								env.macos_enki_DIR = sh ( script: 'dirname $(find $PWD/dist/macos -name enkiConfig.cmake | head -1)', returnStdout: true).trim()
							}
							echo "Found dashel_DIR=${env.dashel_DIR} enki_DIR=${env.enki_DIR}"
							unstash 'source'
							CMake([sourceDir: pwd()+'/aseba', label: 'macos', preloadScript: 'set -x',
								envs: [ "dashel_DIR=${env.macos_dashel_DIR}", "enki_DIR=${env.macos_enki_DIR}" ] ])
							stash includes: 'dist/**', name: 'dist-aseba-macos'
							stash includes: 'build/**', name: 'build-aseba-macos'
						}
					},
					"windows" : {
						node('windows') {
							unstash 'dist-dashel-windows'
							unstash 'dist-enki-windows'
							script {
								env.windows_dashel_DIR = sh ( script: 'dirname $(find $PWD/dist/windows -name dashelConfig.cmake | head -1)', returnStdout: true).trim()
								env.windows_enki_DIR = sh ( script: 'dirname $(find $PWD/dist/windows -name enkiConfig.cmake | head -1)', returnStdout: true).trim()
							}
							unstash 'source'
							CMake([sourceDir: pwd()+'/aseba', label: 'windows', preloadScript: 'set -x',
								envs: [ "dashel_DIR=${env.windows_dashel_DIR}", "enki_DIR=${env.windows_enki_DIR}" ] ])
							stash includes: 'dist/**', name: 'dist-aseba-windows'
							// stash includes: 'build/**', name: 'build-aseba-windows'
						}
					}
				)
			}
		}
		stage('Test') {
			// Only do some tests. To do: add test labels in CMakeLists to distinguish between
			// obligatory smoke tests (to be done for every PR) and extended tests only for
			// releases or for end-to-end testing.
			steps {
				node('debian') {
					unstash 'build-aseba-debian'
					dir('build/debian') {
						sh "LANG=en_US.UTF-8 ctest -E 'e2e.*|simulate.*|.*http.*|valgrind.*'"
					}
				}
			}
		}
		stage('Extended Test') {
			// Extended tests are only run for the master branch.
			when {
				expression {
					return env.BRANCH == 'master'
				}
			}
			steps {
				node('debian') {
					unstash 'build-aseba-debian'
					dir('build/debian') {
						sh "LANG=en_US.UTF-8 ctest -R 'e2e.*|simulate.*|.*http.*|valgrind.*'"
					}
				}
			}
		}
		stage('Package') {
			// Packages are only built for the master branch
			when {
				expression {
					return env.BRANCH == 'master'
				}
			}
			steps {
				parallel (
					"debian-pack" : {
						node('docker') {
							script {
								docker.image('aseba/buildfarm').inside {
									unstash 'source'
									sh '''
										(cd externals/dashel && debuild -i -us -uc -b && sudo dpkg -i ../libdashel*.deb)
										(cd externals/enki && debuild -i -us -uc -b && sudo dpkg -i ../libenki*.deb)
										(cd aseba && debuild -i -us -uc -b)
										exit 0
									'''
								}
							}
							archiveArtifacts artifacts: 'aseba*.deb', fingerprint: true, onlyIfSuccessful: true
						}
					},
					"macos-pack" : {
						node('macos') {
							unstash 'build-aseba-macos'
							git branch: 'inherit-env', url: 'https://github.com/davidjsherman/aseba-osx.git'
							sh '''
								[ -d source ] || ln -s . source
								export your_qt_path=$(otool -L dist/macos/bin/asebastudio | grep QtCore | perl -pe "s{\\s*(/.*)lib/QtCore.*}{\\$1}")
								export your_qwt_path=$(otool -L dist/macos/bin/asebastudio | grep qwt.framework | perl -pe "s{\\s*(/.*)lib/QtCore.*}{\\$1}")
								export your_certificate=none
								mkdir -p build/packager &&
									(cd build/packager && bash -x ../../packager/packager_script && mv Aseba*.dmg ../..)
							'''
							archiveArtifacts artifacts: 'Aseba*.dmg', fingerprint: true, onlyIfSuccessful: true
						}
					}
				)
			}
		}
		stage('Archive') {
			steps {
				// Show how to do this with an array of parallel steps, rather that explictly with parallel()
				script {
					// Can't use collectEntries yet [JENKINS-26481], so use a loop accumulating into an array
					def p = [:]
					for (x in ['debian','macos','windows']) {
						def label = x		// capture loop variable (CPS bug)
						p[label+'-archive'] = {
							node(label) {
								unstash 'dist-aseba-' + label
								archiveArtifacts artifacts: 'dist/**', fingerprint: true, onlyIfSuccessful: true
							}
						}
					}
					parallel p;
				}
			}
		}
	}
}
