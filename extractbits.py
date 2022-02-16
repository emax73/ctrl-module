import fileinput

data = {}
data['D0'] = ''
data['D1'] = ''
for line in fileinput.input():
    h = line.rstrip().split(':')
    if len(h) >= 2 and (h[0] == 'D0' or h[0] == 'D1'):
        for n in h[1].split(' '):
            data[h[0]] = data[h[0]] + (('00000000' + bin(int(n, 16))[2:])[-8:])

#print data

l = len(data['D1'])
last = '?'
out = ''
for i in range(0,l):
    if (last != data['D1'][i]):
        last =  data['D1'][i]
        out = out + data['D0'][i]

outhex = ''
while len(out) >= 8:
    outhex = outhex + '%02X ' % int(out[:8], 2)
    out = out[8:]


print outhex
