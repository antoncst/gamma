.PHONY: clean All

All:
	@echo "----------Building project:[ gamma - Release ]----------"
	@cd "gamma" && "$(MAKE)" -f  "gamma.mk"
clean:
	@echo "----------Cleaning project:[ gamma - Release ]----------"
	@cd "gamma" && "$(MAKE)" -f  "gamma.mk" clean
