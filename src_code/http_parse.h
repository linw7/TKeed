//
// Latest edit by TeeKee on 2017/7/23.
//


#ifndef HTTP_PARSE_H
#define HTTP_PARSE_H

#define CR '\r'
#define LF '\n'

// http请求行解析
int tk_http_parse_request_line(tk_http_request_t *request);
// http请求体解析
int tk_http_parse_request_body(tk_http_request_t *request);

#endif
