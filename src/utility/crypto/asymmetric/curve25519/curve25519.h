/*
 * This file is part of the SSH Library
 *
 * Copyright (c) 2013 by Aris Adamantiadis <aris@badcode.be>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation,
 * version 2.1 of the License.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef _CURVE25519_H_
#define _CURVE25519_H_

#define CURVE25519_PUBKEY_SIZE 32
#define CURVE25519_PRIVKEY_SIZE 32

void crypto_scalarmult_base(unsigned char *q, const unsigned char *n);
void crypto_scalarmult(unsigned char *q, const unsigned char *n, const unsigned char *p);
void curve25519_create_keypair(unsigned char *public_key, unsigned char *private_key);
void curve25519_create_keypair_with_ed25519privkey(unsigned char *public_key, unsigned char *private_key, unsigned char *ed25519_private_key);

#endif /* _CURVE25519_H_ */
