#include "diff.hpp"
#include "../VNode/VNode.hpp"
#include <emscripten.h>
#include <emscripten/val.h>
#include <iterator>
#include <cstdint>
#include <map>

namespace asmdom {

	const std::string emptyString;

	void diffAttrs(VNode* __restrict__ const oldVnode, VNode* __restrict__ const vnode) {
		if (oldVnode->data.attrs.empty() && vnode->data.attrs.empty()) return;

		for (auto& it : oldVnode->data.attrs) {
			if (!vnode->data.attrs.count(it.first)) {
				EM_ASM_({
					window['asmDomHelpers']['domApi']['removeAttribute'](
						$0,
						Module['UTF8ToString']($1)
					);
				}, vnode->elm, it.first.c_str());
			}
		}

		for (auto& it : vnode->data.attrs) {
			if (!oldVnode->data.attrs.count(it.first) || oldVnode->data.attrs[it.first] != it.second) {
				#ifndef ASMDOM_JS_SIDE
					if (it.second == "false") {
						EM_ASM_({
							window['asmDomHelpers']['domApi']['removeAttribute'](
								$0,
								Module['UTF8ToString']($1)
							);
						}, vnode->elm, it.first.c_str());
					} else {
				#endif
					
					EM_ASM_({
						window['asmDomHelpers']['domApi']['setAttribute'](
							$0,
							Module['UTF8ToString']($1),
							Module['UTF8ToString']($2)
						);
					}, vnode->elm, it.first.c_str(), it.second == "true" ? emptyString.c_str() : it.second.c_str());
				
				#ifndef ASMDOM_JS_SIDE
					}
				#endif
			}
		}
	};

	#ifndef ASMDOM_JS_SIDE

	void diffProps(const VNode* __restrict__ const oldVnode, const VNode* __restrict__ const vnode) {
		if (oldVnode->data.props.empty() && vnode->data.props.empty()) return;

		emscripten::val elm = emscripten::val::global("window")["asmDomHelpers"]["nodes"][vnode->elm];

		EM_ASM_({
			window['asmDomHelpers']['nodes'][$0]['asmDomRaws'] = [];
		}, vnode->elm);

		for (auto& it : oldVnode->data.props) {
			if (!vnode->data.props.count(it.first)) {
				elm.set(it.first.c_str(), emscripten::val::undefined());
			}
		}

		for (auto& it : vnode->data.props) {
			EM_ASM_({
				window['asmDomHelpers']['nodes'][$0]['asmDomRaws'].push(Module['UTF8ToString']($1));
			}, vnode->elm, it.first.c_str());

			if (
				!oldVnode->data.props.count(it.first) ||
				!it.second.strictlyEquals(oldVnode->data.props.at(it.first)) ||
				(
					(it.first == "value" || it.first == "checked") && 
					!it.second.strictlyEquals(elm[it.first.c_str()])
				)
			) {
				elm.set(it.first.c_str(), it.second);
			}
		}
	};

	void diffCallbacks(const VNode* __restrict__ const oldVnode, const VNode* __restrict__ const vnode) {
		if (oldVnode->data.callbacks.empty() && vnode->data.callbacks.empty()) return;

		for (auto& it : oldVnode->data.callbacks) {
			if (!vnode->data.callbacks.count(it.first)) {
				EM_ASM_({
					var key = Module['UTF8ToString']($1).replace(/^on/, "");
					var elm = window['asmDomHelpers']['nodes'][$0];
					elm.removeEventListener(
						key,
						window['asmDomHelpers']['eventProxy'],
						false
					);
					delete elm['asmDomEvents'][key];
				}, vnode->elm, it.first.c_str());
			}
		}

		EM_ASM_({
			var elm = window['asmDomHelpers']['nodes'][$0];
			elm.asmDomVNode = $1;
			if (elm['asmDomEvents'] === undefined) {
				elm['asmDomEvents'] = {};
			}
		}, vnode->elm, reinterpret_cast<std::uintptr_t>(vnode));

		for (auto& it : vnode->data.callbacks) {
			if (!oldVnode->data.callbacks.count(it.first)) {
				EM_ASM_({
					var key = Module['UTF8ToString']($1).replace(/^on/, "");
					var elm = window['asmDomHelpers']['nodes'][$0];
					elm.addEventListener(
						key,
						window['asmDomHelpers']['eventProxy'],
						false
					);
					elm['asmDomEvents'][key] = window['asmDomHelpers']['eventProxy'];
				}, vnode->elm, it.first.c_str());
			}
		}
	};

	#endif

	void diff(VNode* __restrict__ const oldVnode, VNode* __restrict__ const vnode) {
		#ifdef ASMDOM_JS_SIDE
			EM_ASM_({
				window['asmDomHelpers']['diff']($0, $1, $2);
			}, reinterpret_cast<std::uintptr_t>(oldVnode), reinterpret_cast<std::uintptr_t>(vnode), vnode->elm);
		#endif

		diffAttrs(oldVnode, vnode);

		#ifndef ASMDOM_JS_SIDE
			diffProps(oldVnode, vnode);
			diffCallbacks(oldVnode, vnode);
		#endif
	};

}
