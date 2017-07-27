//
// Latest edit by TeeKee on 2017/7/23.
//

#include "http.h"

int tk_http_parse_request_line(tk_http_request_t *request){
    enum{
        sw_start = 0,
        sw_method,
        sw_spaces_before_uri,
        sw_after_slash_in_uri,
        sw_http,
        sw_http_H,
        sw_http_HT,
        sw_http_HTT,
        sw_http_HTTP,
        sw_first_major_digit,
        sw_major_digit,
        sw_first_minor_digit,
        sw_minor_digit,
        sw_spaces_after_digit,
        sw_almost_done
    }state;
    state = request->state;

    u_char ch, *p, *m;
    size_t pi;
    for(pi = request->pos; pi < request->last; pi++){
        p = (u_char *)&request->buff[pi % MAX_BUF];
        ch = *p;

        switch(state){
            case sw_start:
                request->request_start = p;
                if(ch == CR || ch == LF)
                    break;
                if((ch < 'A' || ch > 'Z') && ch != '_')
                    return TK_HTTP_PARSE_INVALID_METHOD;
                state = sw_method;
                break;

            case sw_method:
                if(ch == ' '){
                    request->method_end = p;
                    m = request->request_start;
                    switch(p - m){
                        case 3:
                            if(tk_str3_cmp(m, 'G', 'E', 'T', ' ')){
                                request->method = TK_HTTP_GET;
                                break;
                            }
                            break;
                        case 4:
                            if(tk_str3Ocmp(m, 'P', 'O', 'S', 'T')){
                                request->method = TK_HTTP_POST;
                                break;
                            }
                            if(tk_str4cmp(m, 'H', 'E', 'A', 'D')){
                                request->method = TK_HTTP_HEAD;
                                break;
                            }
                            break;
                        default:
                            request->method = TK_HTTP_UNKNOWN;
                            break;
                    }
                    state = sw_spaces_before_uri;
                    break;
                }

                if((ch < 'A' || ch > 'Z') && ch != '_')
                    return TK_HTTP_PARSE_INVALID_METHOD;
                break;

            case sw_spaces_before_uri:
                if(ch == '/'){
                    request->uri_start = p + 1;
                    state = sw_after_slash_in_uri;
                    break;
                }
                switch(ch){
                    case ' ':
                        break;
                    default:
                        return TK_HTTP_PARSE_INVALID_REQUEST;
                }
                break;

            case sw_after_slash_in_uri:
                switch(ch){
                    case ' ':
                        request->uri_end = p;
                        state = sw_http;
                        break;
                    default:
                        break;
                }
                break;

            case sw_http:
                switch(ch){
                    case ' ':
                        break;
                    case 'H':
                        state = sw_http_H;
                        break;
                    default:
                        return TK_HTTP_PARSE_INVALID_REQUEST;
                }
                break;

            case sw_http_H:
                switch(ch){
                    case 'T':
                        state = sw_http_HT;
                        break;
                    default:
                        return TK_HTTP_PARSE_INVALID_REQUEST;
                }
                break;

            case sw_http_HT:
                switch(ch){
                    case 'T':
                        state = sw_http_HTT;
                        break;
                    default:
                        return TK_HTTP_PARSE_INVALID_REQUEST;
                }
                break;

            case sw_http_HTT:
                switch(ch){
                    case 'P':
                        state = sw_http_HTTP;
                        break;
                    default:
                        return TK_HTTP_PARSE_INVALID_REQUEST;
                }
                break;

            case sw_http_HTTP:
                switch(ch){
                    case '/':
                        state = sw_first_major_digit;
                        break;
                    default:
                        return TK_HTTP_PARSE_INVALID_REQUEST;
                }
                break;

            case sw_first_major_digit:
                if(ch < '1' || ch > '9')
                    return TK_HTTP_PARSE_INVALID_REQUEST;
                request->http_major = ch - '0';
                state = sw_major_digit;
                break;

            case sw_major_digit:
                if(ch == '.'){
                    state = sw_first_minor_digit;
                    break;
                }
                if(ch < '0' || ch > '9')
                    return TK_HTTP_PARSE_INVALID_REQUEST;
                request->http_major = request->http_major * 10 + ch - '0';
                break;

            case sw_first_minor_digit:
                if(ch < '0' || ch > '9')
                    return TK_HTTP_PARSE_INVALID_REQUEST;
                request->http_minor = ch - '0';
                state = sw_minor_digit;
                break;

            case sw_minor_digit:
                if(ch == CR){
                    state = sw_almost_done;
                    break;
                }
                if(ch == LF)
                    goto done;
                if(ch == ' '){
                    state = sw_spaces_after_digit;
                    break;
                }
                if(ch < '0' || ch > '9')
                    return TK_HTTP_PARSE_INVALID_REQUEST;
                request->http_minor = request->http_minor * 10 + ch - '0';
                break;

            case sw_spaces_after_digit:
                switch(ch){
                    case ' ':
                        break;
                    case CR:
                        state = sw_almost_done;
                        break;
                    case LF:
                        goto done;
                    default:
                        return TK_HTTP_PARSE_INVALID_REQUEST;
                }
                break;

            case sw_almost_done:
                request->request_end = p - 1;
                switch(ch){
                    case LF:
                        goto done;
                    default:
                        return TK_HTTP_PARSE_INVALID_REQUEST;
                }
        }
    }
    request->pos = pi;
    request->state = state;
    return TK_AGAIN;

    done:
    request->pos = pi + 1;
    if (request->request_end == NULL)
        request->request_end = p;
    request->state = sw_start;
    return 0;
}

int tk_http_parse_request_body(tk_http_request_t *request){
    // 状态列表
    enum{
        sw_start = 0,
        sw_key,
        sw_spaces_before_colon,
        sw_spaces_after_colon,
        sw_value,
        sw_cr,
        sw_crlf,
        sw_crlfcr
    }state;
    state = request->state;

    size_t pi;
    unsigned char ch, *p;
    tk_http_header_t *hd;
    for (pi = request->pos; pi < request->last; pi++) {
        p = (unsigned char*)&request->buff[pi % MAX_BUF];
        ch = *p;

        switch(state){
            case sw_start:
                if(ch == CR || ch == LF)
                    break;
                request->cur_header_key_start = p;
                state = sw_key;
                break;

            case sw_key:
                if(ch == ' '){
                    request->cur_header_key_end = p;
                    state = sw_spaces_before_colon;
                    break;
                }
                if(ch == ':'){
                    request->cur_header_key_end = p;
                    state = sw_spaces_after_colon;
                    break;
                }
                break;

            case sw_spaces_before_colon:
                if(ch == ' ')
                    break;
                else if(ch == ':'){
                    state = sw_spaces_after_colon;
                    break;
                }
                else
                    return TK_HTTP_PARSE_INVALID_HEADER;

            case sw_spaces_after_colon:
                if (ch == ' ')
                    break;
                state = sw_value;
                request->cur_header_value_start = p;
                break;

            case sw_value:
                if(ch == CR){
                    request->cur_header_value_end = p;
                    state = sw_cr;
                }
                if(ch == LF){
                    request->cur_header_value_end = p;
                    state = sw_crlf;
                }
                break;

            case sw_cr:
                if(ch == LF){
                    state = sw_crlf;
                    hd = (tk_http_header_t *) malloc(sizeof(tk_http_header_t));
                    hd->key_start = request->cur_header_key_start;
                    hd->key_end = request->cur_header_key_end;
                    hd->value_start = request->cur_header_value_start;
                    hd->value_end = request->cur_header_value_end;
                    list_add(&(hd->list), &(request->list));
                    break;
                }
                else
                    return TK_HTTP_PARSE_INVALID_HEADER;

            case sw_crlf:
                if(ch == CR)
                    state = sw_crlfcr;
                else{
                    request->cur_header_key_start = p;
                    state = sw_key;
                }
                break;

            case sw_crlfcr:
                switch(ch){
                    case LF:
                        goto done;
                    default:
                        return TK_HTTP_PARSE_INVALID_HEADER;
                }
        }
    }
    request->pos = pi;
    request->state = state;
    return TK_AGAIN;

    done:
    request->pos = pi + 1;
    request->state = sw_start;
    return 0;
}
