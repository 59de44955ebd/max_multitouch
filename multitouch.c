/**
	@file
	multitouch - a max external
	valentin schmidt
*/

#include "ext.h"       // standard Max include, always required
#include "ext_obex.h"  // required for new style Max object

#include <Windows.h>

// 255*6 + 1 -> supports max. 255 touch points
#define MAXLIST 1531

////////////////////////// object struct
typedef struct _multitouch
{
	t_object ob;  // the object itself (must be first)

	void *m_outlet1;
	WNDPROC m_oldproc;
	HWND m_hwnd;
	t_symbol *m_wincaption;
	char m_bGestures;

} t_multitouch;

///////////////////////// function prototypes
void *multitouch_new(t_symbol *s, long argc, t_atom *argv);
void multitouch_free(t_multitouch *x);

void multitouch_start(t_multitouch *x);
void multitouch_stop(t_multitouch *x);

t_max_err multitouch_attr_gestures_set(t_multitouch *x, void *attr, long ac, t_atom *av);

LRESULT CALLBACK MyWndProc(HWND hwnd, UINT umsg, WPARAM wParam, LPARAM lParam);

//////////////////////// global class pointer variable
void *multitouch_class;

void ext_main(void *r)
{
	multitouch_class = class_new("multitouch", (method)multitouch_new, (method)multitouch_free,
			(long)sizeof(t_multitouch), 0L /* leave NULL!! */, A_GIMME, 0);

	class_addmethod(multitouch_class, (method)multitouch_start, "start", 0);
	class_addmethod(multitouch_class, (method)multitouch_stop, "stop", 0);

	CLASS_ATTR_CHAR(multitouch_class, "gestures", 0, t_multitouch, m_bGestures);
	//CLASS_ATTR_ACCESSORS(multitouch_class, "gestures", (method)multitouch_attr_gestures_get, (method)multitouch_attr_gestures_set);
	CLASS_ATTR_ACCESSORS(multitouch_class, "gestures", NULL, (method)multitouch_attr_gestures_set);
	CLASS_ATTR_DEFAULT_SAVE(multitouch_class, "gestures", 0, "0");
	CLASS_ATTR_STYLE_LABEL(multitouch_class, "gestures", 0, "onoff", "Capture gestures");

	class_register(CLASS_BOX, multitouch_class);

	object_post(NULL, "multitouch - © valentin schmidt - compiled %s", __TIMESTAMP__);
}

void multitouch_free(t_multitouch *x)
{
	;
}

void *multitouch_new(t_symbol *s, long argc, t_atom *argv)
{
	t_multitouch *x = NULL;
	if ((x = (t_multitouch *)object_alloc(multitouch_class)))
	{

		x->m_oldproc = NULL;
		x->m_hwnd = NULL;

		if (argc > 0) {
			if ((argv)->a_type == A_SYM)
			{
				x->m_wincaption = atom_getsym(argv);
			}
			else 
			{
				object_error((t_object *)x, "Forbidden argument");
			}
		}

		x->m_bGestures = false;
		x->m_outlet1 = bangout(x);
	}
	return (x);
}

t_max_err multitouch_attr_gestures_set(t_multitouch *x, void *attr, long ac, t_atom *av)
{
	if (ac && av)
	{
		//x->m_bGestures = (char)atom_getlong(av);
		x->m_bGestures = (char)atom_getcharfix(av);

		if (x->m_oldproc && x->m_hwnd)
		{
			if (x->m_bGestures)
			{
				UnregisterTouchWindow(x->m_hwnd);
			}
			else
			if (!RegisterTouchWindow(x->m_hwnd, 0))
			{
				object_error((t_object *)x, "Failed to register window for multi-touch input");
			}

		}
	}
	return MAX_ERR_NONE;
}

void multitouch_start(t_multitouch *x)
{
	if (!x->m_oldproc)
	{
		x->m_hwnd = NULL;

		if (x->m_wincaption)
		{
			x->m_hwnd = FindWindowA(NULL, x->m_wincaption->s_name);
			if (!x->m_hwnd)
			{
				object_error((t_object *)x, "Failed to find window with title '%s'", x->m_wincaption->s_name);
			}
		}

		if (!x->m_hwnd)
		{
			// if no argument, use the patcher window
			object_post((t_object *)x, "No (valid) window title supplied, using patcher window");
			x->m_hwnd = main_get_client();
		}

		if (SetProp(x->m_hwnd, "multitouch", x))
		{
			x->m_oldproc = (WNDPROC)SetWindowLongPtr(x->m_hwnd, GWLP_WNDPROC, (LONG_PTR)MyWndProc);
			if (!x->m_oldproc)
			{
				object_error((t_object *)x, "SetWindowLongPtr failed");
			}
			else
			{
				if (!x->m_bGestures)
				{
					if (!RegisterTouchWindow(x->m_hwnd, 0))
					{
						object_error((t_object *)x, "Failed to register window for multi-touch input");
					}
				}
			}
		}
		else 
		{
			object_error((t_object *)x, "SetProp failed");
		}
	}
}

void multitouch_stop(t_multitouch *x)
{
	if (x->m_oldproc && x->m_hwnd)
	{
		UnregisterTouchWindow(x->m_hwnd);

		(WNDPROC)SetWindowLongPtr(x->m_hwnd, GWLP_WNDPROC, (LONG_PTR)x->m_oldproc);
		x->m_oldproc = NULL;
		x->m_hwnd = NULL;
	}
	else {
		object_error((t_object *)x, "Not running");
	}
}

LRESULT CALLBACK MyWndProc(HWND hwnd, UINT umsg, WPARAM wParam, LPARAM lParam)
{
	t_multitouch *x = (t_multitouch*)GetProp(hwnd, "multitouch");

	if (umsg == WM_TOUCH)
	{
		UINT cInputs = LOWORD(wParam);
		PTOUCHINPUT pInputs = malloc(cInputs * sizeof(TOUCHINPUT));
		if (NULL != pInputs)
		{
			if (GetTouchInputInfo((HTOUCHINPUT)lParam,
				cInputs,
				pInputs,
				sizeof(TOUCHINPUT)))
			{

				//typedef struct tagTOUCHINPUT {
				//	LONG      x;
				//	LONG      y;
				//	HANDLE    hSource;
				//	DWORD     dwID;
				//	DWORD     dwFlags;
				//	DWORD     dwMask;
				//	DWORD     dwTime;
				//	ULONG_PTR dwExtraInfo;
				//	DWORD     cxContact;
				//	DWORD     cyContact;
				//} TOUCHINPUT, *PTOUCHINPUT;

				t_atom pAtomList[MAXLIST];

				//post("cInputs: %d", cInputs);
				if (cInputs > 255) cInputs = 255;
				atom_setlong(pAtomList + 0, (t_atom_long)cInputs);
	
				for (UINT i = 0; i < cInputs; i++)
				{
					atom_setlong(pAtomList + (i * 6) + 1, (t_atom_long)pInputs[i].x);
					atom_setlong(pAtomList + (i * 6) + 2, (t_atom_long)pInputs[i].x);
					atom_setlong(pAtomList + (i * 6) + 3, (t_atom_long)pInputs[i].dwID);
					atom_setlong(pAtomList + (i * 6) + 4, (t_atom_long)pInputs[i].dwFlags);

					atom_setlong(pAtomList + (i * 6) + 5, (t_atom_long)(pInputs[i].dwFlags & TOUCHINPUTMASKF_CONTACTAREA ? pInputs[i].cxContact : 0));
					atom_setlong(pAtomList + (i * 6) + 6, (t_atom_long)(pInputs[i].dwFlags & TOUCHINPUTMASKF_CONTACTAREA ? pInputs[i].cyContact : 0));
				}

				outlet_list(x->m_outlet1, 0L, 1 + cInputs*6, (t_atom *)&pAtomList);

				if (!CloseTouchInputHandle((HTOUCHINPUT)lParam))
				{
					// error handling
				}
			}
			else
			{
				DWORD err = GetLastError();
				object_error((t_object *)x, "GetTouchInputInfo returned error: %ld", err);
			}

			free(pInputs);
		}
		else
		{
			// error handling, presumably out of memory
		}
	}

	else if (umsg == WM_GESTURE)
	{
		GESTUREINFO gestureInfo = { 0 };
		gestureInfo.cbSize = sizeof(gestureInfo);
		BOOL bResult = GetGestureInfo((HGESTUREINFO)lParam, &gestureInfo);

		if (!bResult)
		{
			DWORD err = GetLastError();
			object_error((t_object *)x, "GetGestureInfo returned error: %ld", err);
		}
		else {
			if (gestureInfo.dwID != GID_BEGIN && gestureInfo.dwID != GID_END)
			{
				t_atom myList[5];
				atom_setlong(myList + 0, (t_atom_long)gestureInfo.dwID);
				atom_setlong(myList + 1, (t_atom_long)gestureInfo.dwFlags);
				atom_setlong(myList + 2, (t_atom_long)gestureInfo.ullArguments);
				atom_setlong(myList + 3, (t_atom_long)gestureInfo.ptsLocation.x);
				atom_setlong(myList + 4, (t_atom_long)gestureInfo.ptsLocation.y);
				outlet_list(x->m_outlet1, 0L, 5, (t_atom *)&myList);

				// consume?
				//CloseGestureInfoHandle((HGESTUREINFO)lParam);
				//return 0;
			}

			CloseGestureInfoHandle((HGESTUREINFO)lParam);
		}

	}

	if (x && x->m_oldproc)
		return CallWindowProc(x->m_oldproc, hwnd, umsg, wParam, lParam);
	else
		return DefMDIChildProc(hwnd, umsg, wParam, lParam);
}
