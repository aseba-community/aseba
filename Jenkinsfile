#!groovy

// Jenkinsfile for compiling, testing, and packaging Aseba.
// Requires CMake plugin from https://github.com/davidjsherman/aseba-jenkins.git in global library.

pipeline {
	agent any

	environment {
		VERBOSE = "1"
	}

	// Jenkins will prompt for parameters when a branch is build manually
	parameters {
		string(defaultValue: 'aseba-community/dashel/master', description: 'Dashel project', name: 'project_dashel')
		string(defaultValue: 'enki-community/enki/master', description: 'Enki project', name: 'project_enki')
	}

	// Trigger the build
	triggers {
		// Poll GitHub every two hours, in case webhooks aren't used
		pollSCM('H */2 * * *')
		// Rebuild if Dashel or Enki masters have been rebuilt
		upstream(
			upstreamProjects: 'aseba-community/dashel/master,enki-community/enki/master',
			threshold: hudson.model.Result.SUCCESS
		)
	}
	
	// Everything will be built in the build/ directory.
	// Everything will be installed in the dist/ directory.
	stages {
		stage('Prepare') {
			steps {
				echo "project_dashel=${env.project_dashel}"
				echo "project_enki=${env.project_enki}"

				// Jenkins will automatically check out the source
				// Fixme: Stashed source includes .git otherwise submodule update fails
				sh 'git submodule update --init'

				// Dashel and Enki are retrieved from archived artifacts
				script {
					// Failsafe values for the benefit of automatic jobs
					def pr_dashel = env.project_dashel ?: 'aseba-community/dashel/master'
					def pr_enki = env.project_enki ?: 'enki-community/enki/master'

					def p = ['debian','macos','windows'].collectEntries{
						[ (it): {
								node(it) {
									copyArtifacts projectName: "${pr_dashel}",
										  filter: 'dist/'+it+'/**',
										  selector: lastSuccessful()
									copyArtifacts projectName: "${pr_enki}",
										  filter: 'dist/'+it+'/**',
										  selector: lastSuccessful()
									stash includes: 'dist/**', name: 'dist-externals-' + it
								}
							}
						]
					}
					parallel p;
				}
			}
		}
		stage('Compile') {
			parallel {
				stage("Compile on debian") {
					agent {
						label 'debian'
					}
					steps {
						sh 'git submodule update --init'
						unstash 'dist-externals-debian'
						CMake([label: 'debian'])
						stash includes: 'dist/**', name: 'dist-aseba-debian'
						stash includes: 'build/**', name: 'build-aseba-debian'
					}
				}
				stage("Compile on macos") {
					agent {
						label 'macos'
					}
					steps {
						sh 'git submodule update --init'
						unstash 'dist-externals-macos'
						script {
							env.macos_enki_DIR = sh ( script: 'dirname $(find $PWD/dist/macos -name enkiConfig.cmake | head -1)', returnStdout: true).trim()
							env.macos_dashel_DIR = sh ( script: 'dirname $(find $PWD/dist/macos -name dashelConfig.cmake | head -1)', returnStdout: true).trim()
						}
						echo "macos_enki_DIR=${env.macos_enki_DIR}"
						echo "macos_dashel_DIR=${env.macos_dashel_DIR}"
						CMake([label: 'macos',
							   getCmakeArgs: "-DCMAKE_PREFIX_PATH=/usr/local/opt/qt5",
							   envs: [ "enki_DIR=${env.macos_enki_DIR}", "dashel_DIR=${env.macos_dashel_DIR}" ] ])
						stash includes: 'dist/**', name: 'dist-aseba-macos'
					}
				}
				stage("Compile on windows") {
					agent {
						label 'windows-qt5'
					}
					steps {
						sh 'git submodule update --init'
						CMake([label: 'windows', makeInstallInvocation:'true',
							   getCmakeArgs: "-DCMAKE_PREFIX_PATH=/c/msys32/mingw32/lib"])
					}
				}
			}
		}
		stage('Smoke Test') {
			// Only do some tests. To do: add test labels in CMakeLists to distinguish between
			// obligatory smoke tests (to be done for every PR) and extended tests only for
			// releases or for end-to-end testing.
			parallel {
				stage("Smoke test on debian") {
					agent {
						label 'debian'
					}
					steps {
						unstash 'build-aseba-debian'
						dir('build/debian') {
							sh "LANG=en_US.UTF-8 ctest -T Test -E 'e2e.*|simulate.*|.*http.*|valgrind.*'"
						}
						stash includes: 'build/debian/Testing/**', name: 'test-results', allowEmpty: true
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
			parallel {
				stage("Extended test on debian") {
					agent {
						label 'debian'
					}
					steps {
						unstash 'build-aseba-debian'
						dir('build/debian') {
							sh "LANG=en_US.UTF-8 ctest -T Test -R 'e2e.*|simulate.*|.*http.*|valgrind.*'"
						}
						stash includes: 'build/debian/Testing/**', name: 'test-results', allowEmpty: true
					}
				}
			}
		}
		// No stage('Package'), packaging is handled by a separate job
		stage('Archive') {
			steps {
				unstash 'dist-aseba-debian'
				unstash 'dist-aseba-macos'
				archiveArtifacts artifacts: 'dist/**', fingerprint: true, onlyIfSuccessful: true
			}
		}
	}
	post {
		always {
			unstash 'test-results'
			step([$class: 'XUnitPublisher', testTimeMargin: '3000', thresholdMode: 1, thresholds: [[$class: 'FailedThreshold', failureNewThreshold: '', failureThreshold: '', unstableNewThreshold: '', unstableThreshold: '5'], [$class: 'SkippedThreshold', failureNewThreshold: '', failureThreshold: '', unstableNewThreshold: '', unstableThreshold: '']], tools: [[$class: 'CTestType', deleteOutputFiles: true, failIfNotNew: false, pattern: 'build/debian/Testing/*/Test.xml', skipNoTestFiles: false, stopProcessingIfError: true]]])
		}
	}
}
