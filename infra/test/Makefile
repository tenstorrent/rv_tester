CONTAINER_ARGS                      ?=
PYTHON_ARGS                         ?=
PYTEST_ARGS                         ?=
PARALLEL_RUNNER_ARGS                ?=

# --------------------------------------------------------
# Derived args
# --------------------------------------------------------
var_INFRA_PATH                      := $(realpath $(PWD)/..)
var_UNIT_TESTS                      := $(shell find unit_tests -name "*.py" )
var_UNIT_TESTS_PARALLEL_TARGETS     := $(foreach unit_test, $(var_UNIT_TESTS), --target $(unit_test)-pt)

# --------------------------------------------------------
# container shold be available locally
# Other executables should be available inside container
# --------------------------------------------------------

exec_ENV_CONTAINER_RUNNER             := /usr/bin/env PODMAN_RUN_OPTS_DONT_ADD_WORKDIR=1 PODMAN_RUN_OPTS="-w $(PWD)"
exec_CONTAINER_RUNNER                 := $(exec_ENV_CONTAINER_RUNNER) $(var_INFRA_PATH)/podman-scripts/container-run-tt $(CONTAINER_ARGS)
exec_ENV_PYTHON_RUNNER                := /usr/bin/env PYTHONPATH=$(var_INFRA_PATH):$(PWD)/lib
exec_CONTAINER_PYTHON_RUNNER          := $(exec_CONTAINER_RUNNER) $(exec_ENV_PYTHON_RUNNER) /usr/bin/python3 -B $(PYTHON_ARGS)
exec_CONTAINER_PYTEST_RUNNER          := $(exec_CONTAINER_PYTHON_RUNNER) -m pytest $(PYTEST_ARGS)
exec_CONTAINER_PARALLEL_RUNNER        := $(exec_CONTAINER_PYTHON_RUNNER) $(PWD)/bin/parallel_runner $(PARALLEL_RUNNER_ARGS)
exec_PYTHON_RUNNER                    := $(exec_ENV_PYTHON_RUNNER) /usr/bin/python3 -B $(PYTHON_ARGS)
exec_PYTEST_RUNNER                    := $(exec_PYTHON_RUNNER) -m pytest $(PYTEST_ARGS)

# --------------------------------------------------------
# Functions
# --------------------------------------------------------
define run_parallel
	$(exec_CONTAINER_PARALLEL_RUNNER) $(1)
endef

define run_with_container_python
	$(exec_CONTAINER_PYTHON_RUNNER) $(1)
endef

define run_with_container_pytest
	$(exec_CONTAINER_PYTEST_RUNNER) $(1)
endef

define run_with_python
	$(exec_PYTHON_RUNNER) $(1)
endef

define run_with_pytest
	$(exec_PYTEST_RUNNER) $(1)
endef

# --------------------------------------------------------
#
# Targets
# --------------------------------------------------------
unit_tests-banner:
	@echo '-------------------------------------------'
	@echo '                UNIT TESTS                 '
	@echo '-------------------------------------------'

unit_tests-parallel: unit_tests-banner
	$(call run_parallel, $(var_UNIT_TESTS_PARALLEL_TARGETS))

sanity: unit_tests-parallel
	@echo '-------------------------------------------'
	@echo '           SANITY TESTING DONE             '
	@echo '-------------------------------------------'

clean:
	rm -rf TMP

help:
	@echo "Targets which run more that one tests:"
	@echo "-----------------------------------------------------------"
	T_TESTS_PYTEST_TARGETS@echo "sanity 				- Run all different kinds of tests"
	@echo "unit_tests-parallel  - Run all unit tests in parallel"
	@echo "-----------------------------------------------------------"
	@echo "Single file specific targets:"
	@echo "-----------------------------------------------------------"
	@echo "<file>-py			- Run a file with python"
	@echo "<file>-pt			- Run a file with pytest"
	@echo "<file>-cpy			- Run a file with python within container"
	@echo "<file>-cpt			- Run a file with pytest within container"
	@echo $(var_UNIT_TESTS_PYTEST_TARGETS)

%-cpy:
	$(call run_with_container_python, $*)

%-cpt:
	$(call run_with_container_pytest, $*)

%-pt:
	$(call run_with_pytest, $*)

%-py:
	$(call run_with_python, $*)

