#ifndef GDLOGGER_H
#define GDLOGGER_H

#include "Psionic/include/PSLogger.hpp"
#include "core/error/error_list.h"
#include "core/error/error_macros.h"

class GDLogger : public PSLogger {
	virtual void log(LogType p_type, std::string p_category, std::string p_msg) override {
		if (p_type == LogType::Error || p_type == LogType::Critical) {
			_err_print_error("cpp", "psengine", 0, p_msg.c_str(), ERR_HANDLER_ERROR);
		} else if (p_type == LogType::Warning) {
			_err_print_error("cpp", "psengine", 0, p_msg.c_str(), ERR_HANDLER_WARNING);
		} else {
			print_line(p_msg.c_str());
		}
	};
};

#endif
