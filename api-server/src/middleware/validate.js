const Joi = require('joi')
const { createProblemDetails } = require('./errorHandler')

function validate(schema, source = 'body') {
  return (req, res, next) => {
    const { error, value } = schema.validate(req[source], {
      abortEarly: false,
      stripUnknown: true
    })

    if (error) {
      const errors = error.details.map(d => ({
        field: d.path.join('.'),
        message: d.message,
        code: d.type
      }))

      return res.status(400).json(createProblemDetails({
        type: 'validation-error',
        title: 'Validation Error',
        status: 400,
        detail: 'Request validation failed',
        instance: req.originalUrl,
        errors
      }))
    }

    req[source] = value
    next()
  }
}

const schemas = {
  roomIdParam: Joi.object({
    roomId: Joi.number().integer().positive().required()
  }),

  login: Joi.object({
    username: Joi.string().min(2).max(50).required(),
    password: Joi.string().min(6).max(100).required()
  }),

  sendDanmaku: Joi.object({
    roomId: Joi.number().integer().positive().required(),
    content: Joi.string().min(1).max(500).required(),
    color: Joi.string().regex(/^#[0-9a-fA-F]{6}$/).optional(),
    type: Joi.string().valid('normal', 'emoji').optional()
  }),

  sendGift: Joi.object({
    roomId: Joi.number().integer().positive().required(),
    giftId: Joi.number().integer().positive().required(),
    giftName: Joi.string().max(100).optional().allow(''),
    count: Joi.number().integer().min(1).max(9999).optional(),
    price: Joi.number().positive().optional(),
    effectType: Joi.string().valid('normal', 'rain', 'rocket', 'fire').optional()
  }),

  createRoom: Joi.object({
    name: Joi.string().min(1).max(100).required(),
    hostId: Joi.number().integer().positive().required()
  })
}

module.exports = { validate, schemas }
