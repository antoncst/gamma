.PHONY: clean All

All:
	@echo "----------Building project:[ gamma - Debug ]----------"
	@cd "gamma" && "$(MAKE)" -f  "gamma.mk"
clean:
	@echo "----------Cleaning project:[ gamma - Debug ]----------"
	@cd "gamma" && "$(MAKE)" -f  "gamma.mk" clean
