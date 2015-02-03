#ifndef GRAPEVINE_SRC_GV_BROWSER_HPP_
#define GRAPEVINE_SRC_GV_BROWSER_HPP_

// FIXME cevans87: What should the callback give?
typedef int (*gv_callback)(void);

class GVBrowser
{
    public:
        int enable();
        int disable();
        int register_callback(gv_callback callback);
    private:
        gv_callback _callback;
};

#endif // GRAPEVINE_SRC_GV_BROWSER_HPP_

