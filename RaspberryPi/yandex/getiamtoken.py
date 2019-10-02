import requests, json, os
from datetime import datetime

__location__ = os.path.realpath(os.path.join(os.getcwd(), os.path.dirname(__file__)))

def save_token(jsonobj):
    with open(os.path.join(__location__, 'iamtoken.json'), 'w') as f:
        json.dump(jsonobj, f)
    print('new token saved')

def is_expired(datestr):
    try:
        tokendate = datetime.strptime(datestr, '%Y-%m-%dT%H:%M:%S.%fZ')
        expire = (datetime.now() > tokendate)
    except:
        print('error parsing expiration date')
        expire = True
    return expire

def get_new_token():
    with open(os.path.join(__location__, 'private.json'), 'r') as f:
        private = json.load(f)
    url = 'https://iam.api.cloud.yandex.net/iam/v1/tokens'
    headers = {'Content-type': 'application/json'}
    data = {"yandexPassportOauthToken":private['OauthToken']}

    resp = requests.post(url, data=json.dumps(data), headers=headers)
    if resp.ok:
        jresp = resp.json()
        token = jresp['iamToken']
        save_token(jresp)
    else:
        print('resp is not ok')
        print(resp)
        token = None
    return token

def get_token():
    try:
        with open(os.path.join(__location__, 'iamtoken.json'), 'r') as f:
            jtoken = json.load(f)
        if is_expired(jtoken['expiresAt']):
            print('token is expired, taking new one ...')
            token = get_new_token()
        else:
            print('existing token')
            token = jtoken['iamToken']
    except Exception as e:
        print('get_token exception:')
        print(e)
        token = get_new_token()
    return token

if __name__ == '__main__':
    print(get_token())